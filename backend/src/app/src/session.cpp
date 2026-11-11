#include <elle/app/session.hpp>

#include <elle/app/protocol.hpp>
#include <elle/app/settings.hpp>
#include <elle/app/task_worker.hpp>
#include <elle/core/backplot.hpp>
#include <elle/core/cycle.hpp>
#include <elle/core/lathe_controller.hpp>
#include <elle/core/threading.hpp>
#include <elle/core/turning.hpp>

#include <cmath>
#include <expected>
#include <filesystem>
#include <format>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace elle::app {

namespace {

/// How far the tool may have moved between preview and start before the preview is stale, mm.
constexpr double max_drift = 0.01;

template <class... Handlers>
struct Overloaded : Handlers... {
    using Handlers::operator()...;
};

template <class... Handlers>
Overloaded(Handlers...) -> Overloaded<Handlers...>;

auto generated(const std::expected<core::GcodeLines, core::CycleError>& program)
    -> std::optional<core::GcodeLines>
{
    return program.has_value() ? std::optional{*program} : std::nullopt;
}

auto error_of(const std::expected<core::GcodeLines, core::CycleError>& program) -> std::string
{
    return program.has_value() ? std::string{} : std::string{core::describe(program.error())};
}

} // namespace

Session::Session(Settings settings, std::filesystem::path settings_file)
    : settings_{std::move(settings)}
    , settings_file_{std::move(settings_file)}
    , controller_{settings_.machine}
{
}

auto Session::handle(std::string_view json, Clock::time_point now) -> Outcome
{
    Outcome outcome{};
    auto command = parse_command(json);
    if (!command.has_value()) {
        outcome.replies.push_back(
            notice_message(NoticeLevel::error, std::format("Bad message: {}", command.error())));
        return outcome;
    }
    const std::scoped_lock lock{mutex_};
    apply(*command, now, outcome);
    return outcome;
}

auto Session::apply(const Command& command, Clock::time_point now, Outcome& outcome) -> void
{
    std::visit(
        Overloaded{
            [&](const Ready&) {
                outcome.replies.push_back(settings_message(settings_));
            },
            [&](const SelectMode& message) {
                controller_.select_mode(message.mode);
                pending_.reset();
            },
            [&](const SetFeed& message) { controller_.set_feed(message.feed); },
            [&](const SetDirection& message) { controller_.set_direction(message.direction); },
            [&](const SetPitches& message) {
                controller_.set_pitches(message.pitches);
                save(outcome);
            },
            [&](const SetAxis& message) {
                controller_.set_axis(message.axis, message.value);
                pending_.reset();
            },
            [&](const JogCommand& message) {
                if (message.held) {
                    controller_.jog_hold(message.button, now);
                } else {
                    controller_.jog_release(message.button);
                }
            },
            [&](const JogReleaseAll&) { controller_.jog_release_all(); },
            [&](const SelectTool& message) {
                controller_.select_tool(message.id);
                pending_.reset();
                save(outcome);
            },
            [&](const SetTools& message) {
                controller_.set_tools(message.tools);
                pending_.reset();
                save(outcome);
            },
            [&](const SetPreferences& message) {
                const Preferences& preferences = message.preferences;
                settings_.diameter_mode = preferences.diameter_mode;
                settings_.metric_on_startup = preferences.metric_on_startup;
                settings_.thread_preset_tab = preferences.thread_preset_tab;
                settings_.turning_preset_tab = preferences.turning_preset_tab;
                settings_.pitch_preset_tabs = preferences.pitch_preset_tabs;
                controller_.set_encoder_scales(preferences.encoder_scale_z,
                                               preferences.encoder_scale_x);
                save(outcome);
            },
            [&](const PlanThreading& message) {
                const auto position = controller_.work_position();
                const auto issues = core::validate_threading(message.request);
                if (!issues.empty()) {
                    outcome.replies.push_back(plan_message(
                        plan(CycleKind::threading, issues, {}, {}, {}, position)));
                    return;
                }
                const auto program = core::plan_threading(message.request, position);
                const auto machine = core::threading_gcode(program, core::Destination::machine);
                const auto preview = core::threading_gcode(program, core::Destination::preview);
                outcome.replies.push_back(plan_message(
                    plan(CycleKind::threading, {}, generated(machine), generated(preview),
                         error_of(machine), position)));
            },
            [&](const PlanTurning& message) {
                const auto position = controller_.work_position();
                const auto issues = core::validate_turning(message.request, position);
                if (!issues.empty()) {
                    outcome.replies.push_back(
                        plan_message(plan(CycleKind::turning, issues, {}, {}, {}, position)));
                    return;
                }
                const auto program = core::plan_turning(message.request, position);
                const auto machine = core::turning_gcode(program, core::Destination::machine);
                const auto preview = core::turning_gcode(program, core::Destination::preview);
                outcome.replies.push_back(plan_message(
                    plan(CycleKind::turning, {}, generated(machine), generated(preview),
                         error_of(machine), position)));
            },
            [&](const StartCycle&) { start_cycle(outcome); },
            [&](const AbortCycle&) {
                pending_.reset();
                outcome.jobs.emplace_back(AbortTask{});
            },
            [&](const Quit&) {
                controller_.jog_release_all();
                outcome.quit = true;
            },
        },
        command);
}

auto Session::plan(CycleKind kind, const core::ParameterIssues& issues,
                   const std::optional<core::GcodeLines>& machine,
                   const std::optional<core::GcodeLines>& preview, std::string error,
                   const core::WorkPosition& start) -> CyclePlan
{
    CyclePlan result{.kind = kind,
                     .issues = issues,
                     .error = std::move(error),
                     .gcode = {},
                     .backplot = std::nullopt,
                     .start = {.x = start.x, .z = start.z}};
    pending_.reset();
    if (!issues.empty() || !machine.has_value() || !preview.has_value()) {
        return result;
    }
    auto plot = core::backplot(*preview, result.start);
    if (!plot.has_value()) {
        result.error = std::format("preview line {}: {}", plot.error().line + 1, plot.error().message);
        return result;
    }
    result.gcode = *machine;
    result.backplot = std::move(*plot);
    pending_ = Pending{.program = *machine, .planned_at = start};
    return result;
}

auto Session::start_cycle(Outcome& outcome) -> void
{
    const auto refuse = [&](std::string_view why) {
        outcome.replies.push_back(notice_message(NoticeLevel::error, why));
    };
    if (!pending_.has_value()) {
        refuse("Preview the cycle before starting it.");
        return;
    }
    if (controller_.mode() != core::OperatingMode::cycle) {
        refuse("Cycles only run on the Canned Cycles page.");
        return;
    }
    if (task_.program_running) {
        refuse("A cycle is already running.");
        return;
    }
    const auto here = controller_.work_position();
    if (std::abs(here.x - pending_->planned_at.x) > max_drift ||
        std::abs(here.z - pending_->planned_at.z) > max_drift) {
        pending_.reset();
        refuse("The tool moved since the preview. Preview the cycle again.");
        return;
    }
    outcome.jobs.emplace_back(RunCycle{.program = std::move(pending_->program)});
    pending_.reset();
}

auto Session::save(Outcome& outcome) -> void
{
    settings_.machine = controller_.settings();
    outcome.replies.push_back(settings_message(settings_));
    if (const auto saved = save_settings(settings_, settings_file_); !saved.has_value()) {
        outcome.replies.push_back(
            notice_message(NoticeLevel::error, std::format("Settings not saved: {}", saved.error())));
    }
}

auto Session::tick(const core::MachineSample& sample, Clock::time_point now) -> Tick
{
    const std::scoped_lock lock{mutex_};
    const auto result = controller_.tick(sample, now);
    Tick tick{.outputs = result.outputs, .job = std::nullopt};
    if (result.resync_motion) {
        tick.job = ResyncMotion{};
    }
    return tick;
}

auto Session::observe(const TaskSnapshot& task) -> void
{
    const std::scoped_lock lock{mutex_};
    task_ = task;
}

auto Session::status() const -> std::string
{
    const std::scoped_lock lock{mutex_};
    return status_message(MachineStatus{
        .readout = controller_.readout(),
        .mode = controller_.mode(),
        .feed = controller_.feed(),
        .direction = controller_.direction(),
        .pitches = controller_.pitches(),
        .leadscrew = controller_.leadscrew(),
        .task = {.connected = task_.connected,
                 .program_running = task_.program_running,
                 .error = task_.error},
    });
}

} // namespace elle::app

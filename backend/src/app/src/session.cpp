#include <elle/app/session.hpp>

#include <elle/app/protocol.hpp>
#include <elle/app/settings.hpp>
#include <elle/app/task_worker.hpp>
#include <elle/core/backplot.hpp>
#include <elle/core/cycle.hpp>
#include <elle/core/cycle_form.hpp>
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
#include <utility>
#include <variant>
#include <vector>

namespace elle::app {

namespace {

/// How far the tool may have moved between preview and start before the preview is stale, mm.
constexpr double max_drift = 0.01;

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
    const std::scoped_lock<std::mutex> lock{mutex_};
    std::visit([&](const auto& message) { on(message, now, outcome); }, *command);
    return outcome;
}

auto Session::on(const Ready& /*message*/, Clock::time_point /*now*/, Outcome& outcome) -> void
{
    outcome.replies.push_back(settings_message(settings_));
}

auto Session::on(const SelectMode& message, Clock::time_point /*now*/, Outcome& /*outcome*/)
    -> void
{
    controller_.select_mode(message.mode);
    pending_.reset();
}

auto Session::on(const SetFeed& message, Clock::time_point /*now*/, Outcome& /*outcome*/) -> void
{
    controller_.set_feed(message.feed);
}

auto Session::on(const SetDirection& message, Clock::time_point /*now*/, Outcome& /*outcome*/)
    -> void
{
    controller_.set_direction(message.direction);
}

auto Session::on(const SetPitches& message, Clock::time_point /*now*/, Outcome& outcome) -> void
{
    controller_.set_pitches(message.pitches);
    save(outcome);
}

auto Session::on(const SetAxis& message, Clock::time_point /*now*/, Outcome& /*outcome*/) -> void
{
    controller_.set_axis(message.axis, message.value);
    pending_.reset();
}

auto Session::on(const JogCommand& message, Clock::time_point now, Outcome& /*outcome*/) -> void
{
    if (message.held) {
        controller_.jog_hold(message.button, now);
    } else {
        controller_.jog_release(message.button);
    }
}

auto Session::on(const JogReleaseAll& /*message*/, Clock::time_point /*now*/,
                 Outcome& /*outcome*/) -> void
{
    controller_.jog_release_all();
}

auto Session::on(const SelectTool& message, Clock::time_point /*now*/, Outcome& outcome) -> void
{
    controller_.select_tool(message.id);
    pending_.reset();
    save(outcome);
}

auto Session::on(const SetTools& message, Clock::time_point /*now*/, Outcome& outcome) -> void
{
    controller_.set_tools(message.tools);
    pending_.reset();
    save(outcome);
}

auto Session::on(const SetPreferences& message, Clock::time_point /*now*/, Outcome& outcome)
    -> void
{
    const Preferences& preferences = message.preferences;
    settings_.diameter_mode = preferences.diameter_mode;
    settings_.metric_on_startup = preferences.metric_on_startup;
    settings_.thread_preset_tab = preferences.thread_preset_tab;
    settings_.turning_preset_tab = preferences.turning_preset_tab;
    settings_.pitch_preset_tabs = preferences.pitch_preset_tabs;
    settings_.back_tool_post = preferences.back_tool_post;
    controller_.set_encoder_scales(preferences.encoder_scale_z, preferences.encoder_scale_x);
    save(outcome);
}

auto Session::on(const PlanThreading& message, Clock::time_point /*now*/, Outcome& outcome)
    -> void
{
    const auto position = controller_.work_position();
    auto [request, issues] = core::threading_request(message.form, position);
    Generated generated{};
    if (issues.empty()) {
        const auto program = core::plan_threading(request, position);
        generated = {.machine = core::threading_gcode(program, core::Destination::machine),
                     .preview = core::threading_gcode(program, core::Destination::preview)};
    }
    outcome.replies.push_back(
        plan_message(plan(CycleKind::threading, std::move(issues), generated, position)));
}

auto Session::on(const PlanTurning& message, Clock::time_point /*now*/, Outcome& outcome) -> void
{
    const auto position = controller_.work_position();
    auto [request, issues] = core::turning_request(message.form, position);
    Generated generated{};
    if (issues.empty()) {
        const auto program = core::plan_turning(request, position);
        const double chamfer = message.form.chamfer.value_or(0.0);
        generated = {
            .machine = core::turning_gcode(program, core::Destination::machine, chamfer),
            .preview = core::turning_gcode(program, core::Destination::preview, chamfer)};
    }
    outcome.replies.push_back(
        plan_message(plan(CycleKind::turning, std::move(issues), generated, position)));
}

auto Session::on(const StartCycle& /*message*/, Clock::time_point /*now*/, Outcome& outcome)
    -> void
{
    start_cycle(outcome);
}

auto Session::on(const AbortCycle& /*message*/, Clock::time_point /*now*/, Outcome& outcome)
    -> void
{
    pending_.reset();
    outcome.jobs.emplace_back(AbortTask{});
}

auto Session::on(const Quit& /*message*/, Clock::time_point /*now*/, Outcome& outcome) -> void
{
    controller_.jog_release_all();
    outcome.quit = true;
}

auto Session::plan(CycleKind kind, core::ParameterIssues issues, const Generated& generated,
                   const core::WorkPosition& start) -> CyclePlan
{
    CyclePlan result{.kind = kind,
                     .issues = std::move(issues),
                     .error = {},
                     .gcode = {},
                     .backplot = std::nullopt,
                     .start = {.x = start.x, .z = start.z}};
    pending_.reset();
    if (!result.issues.empty()) {
        return result;
    }
    if (!generated.machine.has_value() || !generated.preview.has_value()) {
        const auto error = generated.machine.has_value() ? generated.preview.error()
                                                         : generated.machine.error();
        result.error = std::string{core::describe(error)};
        return result;
    }
    auto plot = core::backplot(*generated.preview, result.start);
    if (!plot.has_value()) {
        result.error =
            std::format("preview line {}: {}", plot.error().line + 1, plot.error().message);
        return result;
    }
    result.gcode = *generated.machine;
    result.backplot = std::move(*plot);
    pending_ = Pending{.program = *generated.machine, .planned_at = start};
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
    const std::scoped_lock<std::mutex> lock{mutex_};
    const auto result = controller_.tick(sample, now);
    Tick tick{.outputs = result.outputs, .job = std::nullopt};
    if (result.resync_motion) {
        tick.job = ResyncMotion{};
    }
    return tick;
}

auto Session::observe(const TaskSnapshot& task) -> void
{
    const std::scoped_lock<std::mutex> lock{mutex_};
    task_ = task;
}

auto Session::status() const -> std::string
{
    const std::scoped_lock<std::mutex> lock{mutex_};
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

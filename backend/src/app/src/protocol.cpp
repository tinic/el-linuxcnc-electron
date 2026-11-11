#include <elle/app/protocol.hpp>

#include <elle/app/settings.hpp>
#include <elle/core/backplot.hpp>
#include <elle/core/jog.hpp>
#include <elle/core/lathe_controller.hpp>
#include <elle/core/leadscrew.hpp>

#include <glaze/glaze.hpp>

#include <expected>
#include <format>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

template <>
struct glz::meta<elle::core::OperatingMode> {
    using enum elle::core::OperatingMode;
    static constexpr auto value = glz::enumerate(manual, cycle, other);
};

template <>
struct glz::meta<elle::core::FeedMode> {
    using enum elle::core::FeedMode;
    static constexpr auto value =
        glz::enumerate(longitudinal, cross, front_compound, back_compound);
};

template <>
struct glz::meta<elle::core::DirectionMode> {
    using enum elle::core::DirectionMode;
    static constexpr auto value = glz::enumerate(forward, reverse, hold, idle);
};

template <>
struct glz::meta<elle::core::JogButton> {
    using enum elle::core::JogButton;
    static constexpr auto value = glz::enumerate(x_minus, x_plus, z_minus, z_plus);
};

template <>
struct glz::meta<elle::core::Axis> {
    using enum elle::core::Axis;
    static constexpr auto value = glz::enumerate(x, z, a);
};

template <>
struct glz::meta<elle::core::MoveKind> {
    using enum elle::core::MoveKind;
    static constexpr auto value = glz::enumerate(rapid, feed, thread);
};

template <>
struct glz::meta<elle::app::CycleKind> {
    using enum elle::app::CycleKind;
    static constexpr auto value = glz::enumerate(threading, turning);
};

template <>
struct glz::meta<elle::app::NoticeLevel> {
    using enum elle::app::NoticeLevel;
    static constexpr auto value = glz::enumerate(info, error);
};

namespace elle::app {

// Not an anonymous namespace: glaze's reflection needs these types to have external linkage.
namespace wire {

struct Envelope {
    std::string type{};
};

struct Lenient : glz::opts {
    bool error_on_unknown_keys = false;
};

struct Strict : glz::opts {
    bool error_on_missing_keys = true;
};

struct StatusEnvelope {
    std::string_view type{"status"};
    MachineStatus status{};
};

struct SettingsEnvelope {
    std::string_view type{"settings"};
    Settings settings{};
};

struct LogEnvelope {
    std::string_view type{"log"};
    std::vector<std::string> lines{};
};

struct PlanEnvelope {
    std::string_view type{"plan"};
    CyclePlan plan{};
};

struct NoticeEnvelope {
    std::string_view type{"notice"};
    NoticeLevel level{};
    std::string_view text{};
};

} // namespace wire

namespace {

using wire::Lenient;
using wire::Strict;

template <class T>
auto read_as(std::string_view json) -> std::expected<Command, std::string>
{
    T command{};
    if (const auto error = glz::read<Strict{}>(command, json); error) {
        return std::unexpected{glz::format_error(error, json)};
    }
    return Command{std::move(command)};
}

template <class Message>
auto write(const Message& message) -> std::string
{
    std::string json{};
    // Absent optionals are sent as null: the panel's types say "value or null", never "missing".
    if (glz::write<glz::opts{.skip_null_members = false}>(message, json)) {
        json.clear();
    }
    return json;
}

} // namespace

auto parse_command(std::string_view json) -> std::expected<Command, std::string>
{
    wire::Envelope envelope{};
    if (const auto error = glz::read<Lenient{}>(envelope, json); error) {
        return std::unexpected{glz::format_error(error, json)};
    }
    const std::string_view type = envelope.type;
    if (type == "ready") { return read_as<Ready>(json); }
    if (type == "select_mode") { return read_as<SelectMode>(json); }
    if (type == "set_feed") { return read_as<SetFeed>(json); }
    if (type == "set_direction") { return read_as<SetDirection>(json); }
    if (type == "set_pitches") { return read_as<SetPitches>(json); }
    if (type == "set_axis") { return read_as<SetAxis>(json); }
    if (type == "jog") { return read_as<JogCommand>(json); }
    if (type == "jog_release_all") { return read_as<JogReleaseAll>(json); }
    if (type == "select_tool") { return read_as<SelectTool>(json); }
    if (type == "set_tools") { return read_as<SetTools>(json); }
    if (type == "set_preferences") { return read_as<SetPreferences>(json); }
    if (type == "plan_threading") { return read_as<PlanThreading>(json); }
    if (type == "plan_turning") { return read_as<PlanTurning>(json); }
    if (type == "start_cycle") { return read_as<StartCycle>(json); }
    if (type == "abort_cycle") { return read_as<AbortCycle>(json); }
    if (type == "quit") { return read_as<Quit>(json); }
    return std::unexpected{std::format("unknown message type '{}'", type)};
}

auto status_message(const MachineStatus& status) -> std::string
{
    return write(wire::StatusEnvelope{.status = status});
}

auto settings_message(const Settings& settings) -> std::string
{
    return write(wire::SettingsEnvelope{.settings = settings});
}

auto log_message(const std::vector<std::string>& lines) -> std::string
{
    return write(wire::LogEnvelope{.lines = lines});
}

auto plan_message(const CyclePlan& plan) -> std::string
{
    return write(wire::PlanEnvelope{.plan = plan});
}

auto notice_message(NoticeLevel level, std::string_view text) -> std::string
{
    return write(wire::NoticeEnvelope{.level = level, .text = text});
}

} // namespace elle::app

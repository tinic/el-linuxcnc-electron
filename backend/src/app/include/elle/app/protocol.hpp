#pragma once

// Messages between the operator panel (ui/src/protocol.ts) and the elle executable. Field names
// and enum spellings are the JSON keys and strings, one to one.

#include <elle/app/settings.hpp>
#include <elle/core/backplot.hpp>
#include <elle/core/cycle.hpp>
#include <elle/core/jog.hpp>
#include <elle/core/lathe_controller.hpp>
#include <elle/core/leadscrew.hpp>
#include <elle/core/threading.hpp>
#include <elle/core/turning.hpp>

#include <array>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace elle::app {

// ---- panel to machine ---------------------------------------------------------------------
// Each command keeps the message's "type" so it can be read strictly: unknown keys are errors.

struct Ready {
    std::string type{};
};
struct SelectMode {
    std::string type{};
    core::OperatingMode mode{};
};
struct SetFeed {
    std::string type{};
    core::FeedMode feed{};
};
struct SetDirection {
    std::string type{};
    core::DirectionMode direction{};
};
struct SetPitches {
    std::string type{};
    core::Pitches pitches{};
};
struct SetAxis {
    std::string type{};
    core::Axis axis{};
    double value{};
};
struct JogCommand {
    std::string type{};
    core::JogButton button{};
    bool held{};
};
struct JogReleaseAll {
    std::string type{};
};
struct SelectTool {
    std::string type{};
    int id{};
};
struct SetTools {
    std::string type{};
    std::vector<core::Tool> tools{};
};
struct Preferences {
    bool diameter_mode{};
    bool metric_on_startup{};
    int thread_preset_tab{};
    int turning_preset_tab{};
    std::array<int, 2> pitch_preset_tabs{};
    double encoder_scale_z{};
    double encoder_scale_x{};
};
struct SetPreferences {
    std::string type{};
    Preferences preferences{};
};
struct PlanThreading {
    std::string type{};
    core::ThreadingRequest request{};
};
struct PlanTurning {
    std::string type{};
    core::TurningRequest request{};
};
struct StartCycle {
    std::string type{};
};
struct AbortCycle {
    std::string type{};
};
struct Quit {
    std::string type{};
};

using Command = std::variant<Ready, SelectMode, SetFeed, SetDirection, SetPitches, SetAxis,
                             JogCommand, JogReleaseAll, SelectTool, SetTools, SetPreferences,
                             PlanThreading, PlanTurning, StartCycle, AbortCycle, Quit>;

/// Parses one message from the panel. Unknown types and malformed fields are errors.
[[nodiscard]] auto parse_command(std::string_view json) -> std::expected<Command, std::string>;

// ---- machine to panel ---------------------------------------------------------------------

struct TaskState {
    bool connected{};
    bool program_running{};
    bool error{};
};

struct MachineStatus {
    core::Readout readout{};
    core::OperatingMode mode{};
    core::FeedMode feed{};
    core::DirectionMode direction{};
    core::Pitches pitches{};
    core::LeadscrewOutputs leadscrew{};
    TaskState task{};
};

enum class CycleKind : std::uint8_t {
    threading,
    turning,
};

struct CyclePlan {
    CycleKind kind{};
    core::ParameterIssues issues{};
    std::string error{};
    core::GcodeLines gcode{};
    std::optional<core::Backplot> backplot{};
    core::PlotPoint start{};
};

enum class NoticeLevel : std::uint8_t {
    info,
    error,
};

[[nodiscard]] auto status_message(const MachineStatus& status) -> std::string;
[[nodiscard]] auto settings_message(const Settings& settings) -> std::string;
[[nodiscard]] auto log_message(const std::vector<std::string>& lines) -> std::string;
[[nodiscard]] auto plan_message(const CyclePlan& plan) -> std::string;
[[nodiscard]] auto notice_message(NoticeLevel level, std::string_view text) -> std::string;

} // namespace elle::app

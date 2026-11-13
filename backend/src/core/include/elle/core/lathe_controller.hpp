#pragma once

#include <elle/core/cycle.hpp>
#include <elle/core/jog.hpp>
#include <elle/core/leadscrew.hpp>

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace elle::core {

/// One reading of the `lathe.*` HAL input pins.
struct MachineSample {
    double position_z{};   ///< Z slide, from its linear encoder, mm
    double position_x{};   ///< X slide (radius), from its linear encoder, mm
    double spindle_revs{}; ///< spindle position, revolutions
    double spindle_rps{};  ///< spindle speed, revolutions per second
    double stepper_z{};    ///< Z step generator position feedback, mm
    double stepper_x{};    ///< X step generator position feedback, mm
};

/// Everything written to the `lathe.*` HAL output pins.
struct PinOutputs {
    bool control_source{};
    double forward_z{};
    double forward_x{};
    bool enable_z{};
    bool enable_x{};
    bool enable_stepper_z{};
    bool enable_stepper_x{};
    double offset_z_encoder{};
    double offset_z_stepper{};
    double offset_x_encoder{};
    double offset_x_stepper{};
    bool control_z_type{};
    bool control_x_type{};
    double velocity_z_cmd{};
    double velocity_x_cmd{};
    std::uint32_t reset_z{};
    std::uint32_t reset_x{};
    double scale_encoder_z{};
    double scale_encoder_x{};
};

/// Which part of the machine the operator is driving.
enum class OperatingMode : std::uint8_t {
    manual, ///< electronic leadscrew from the panel
    cycle,  ///< LinuxCNC's motion controller runs canned cycles
    other,  ///< a page without machine control; outputs keep their last state
};

enum class Axis : std::uint8_t {
    x,
    z,
    a,
};

struct Tool {
    int id{};
    double offset_x{}; ///< radius, mm, relative to the reference tool
    double offset_z{}; ///< mm, relative to the reference tool
    std::string description{};
};

/// Linear encoder resolution of the original machine, mm per count (X counts the other way).
inline constexpr double default_encoder_scale = 0.001;
/// Leadscrew feed until the operator picks one, mm per spindle revolution.
inline constexpr double default_pitch = 0.1;

/// Persistent machine configuration the controller needs.
struct MachineSettings {
    double encoder_scale_z{default_encoder_scale};
    double encoder_scale_x{-default_encoder_scale};
    Pitches pitches{.x = default_pitch, .z = default_pitch};
    std::vector<Tool> tools{};
    int current_tool{};
};

/// What the operator sees: work coordinates of the current tool's tip.
struct Readout {
    double x{};          ///< radius, mm
    double z{};          ///< mm
    double a_degrees{};  ///< spindle angle in [0, 360)
    double rpm{};        ///< smoothed spindle speed
};

/// Result of one control tick.
struct TickResult {
    PinOutputs outputs{};
    /// The encoders were zeroed; LinuxCNC's motion controller must re-sync its commanded position
    /// (MDI mode, machine off/on, interpreter reset), as the original did on every mode change.
    bool resync_motion{};
};

/// The machine-side state of the lathe, independent of HAL and the user interface.
///
/// Work coordinates are kept as offsets from the encoder positions. The encoders are zeroed on
/// every change of operating mode (keeping machine coordinates near zero and inside LinuxCNC's
/// soft limits); the offsets absorb that so the readout never moves.
class LatheController {
public:
    using Clock = std::chrono::steady_clock;

    explicit LatheController(MachineSettings settings);

    auto select_mode(OperatingMode mode) -> void;
    auto set_feed(FeedMode feed) -> void;
    auto set_direction(DirectionMode direction) -> void;
    auto set_pitches(const Pitches& pitches) -> void;
    auto set_encoder_scales(double scale_z, double scale_x) -> void;

    /// Touch-off: make the current tool's readout on `axis` read `value` (radius mm, mm, degrees).
    auto set_axis(Axis axis, double value) -> void;

    auto set_tools(std::vector<Tool> tools) -> void;
    auto select_tool(int tool_id) -> void;

    auto jog_hold(JogButton button, Clock::time_point now) -> void;
    auto jog_release(JogButton button) -> void;
    auto jog_release_all() -> void;

    /// Runs one control step against a fresh sample. Call at a fixed rate (100 Hz).
    [[nodiscard]] auto tick(const MachineSample& sample, Clock::time_point now) -> TickResult;

    [[nodiscard]] auto readout() const -> Readout;
    /// Tool-tip position in work coordinates for planning a cycle (A in revolutions).
    [[nodiscard]] auto work_position() const -> WorkPosition;

    [[nodiscard]] auto mode() const -> OperatingMode { return mode_; }
    [[nodiscard]] auto feed() const -> FeedMode { return feed_; }
    [[nodiscard]] auto direction() const -> DirectionMode { return direction_; }
    [[nodiscard]] auto pitches() const -> const Pitches& { return settings_.pitches; }
    [[nodiscard]] auto settings() const -> const MachineSettings& { return settings_; }
    [[nodiscard]] auto leadscrew() const -> const LeadscrewOutputs& { return leadscrew_; }

private:
    /// Encoder zeroing in flight: the encoder positions just before it, and how many ticks remain
    /// until the encoder component has certainly applied it.
    struct PendingReset {
        double position_z{};
        double position_x{};
        int ticks_left{};
    };

    [[nodiscard]] auto tool_offset_x() const -> double;
    [[nodiscard]] auto tool_offset_z() const -> double;
    auto update_leadscrew() -> void;
    auto relatch(const MachineSample& sample) -> void;
    auto apply_jog(const JogVelocities& velocities) -> void;
    auto advance_reset() -> bool;
    auto smooth_rpm(const MachineSample& sample, Clock::time_point now) -> void;

    MachineSettings settings_{};
    OperatingMode mode_{OperatingMode::other};
    FeedMode feed_{FeedMode::longitudinal};
    /// Idle at start-up: stepper drivers unpowered, nothing follows the spindle until the operator
    /// chooses a direction.
    DirectionMode direction_{DirectionMode::idle};
    LeadscrewOutputs leadscrew_{};
    Jog jog_{};
    PinOutputs outputs_{};
    MachineSample sample_{};
    double offset_x_{};
    double offset_z_{};
    double offset_a_{};
    std::optional<PendingReset> pending_reset_{};
    bool reset_requested_{};
    bool relatch_requested_{};
    bool jogging_x_{};
    bool jogging_z_{};
    double rpm_{};
    std::optional<Clock::time_point> last_tick_{};
};

} // namespace elle::core

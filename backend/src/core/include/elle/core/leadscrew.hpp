#pragma once

#include <cstdint>

namespace elle::core {

/// Which slide the leadscrew drives in manual mode.
enum class FeedMode : std::uint8_t {
    longitudinal,
    cross,
    front_compound,
    back_compound,
};

/// What the engaged axes do while the spindle turns.
enum class DirectionMode : std::uint8_t {
    forward, ///< steppers follow the spindle towards the chuck (or inward)
    reverse, ///< steppers follow the spindle away from the chuck (or outward)
    hold,    ///< stepper drivers powered and holding position, not following the spindle
    idle,    ///< stepper drivers unpowered, the slides can be moved by hand
};

/// Feed per spindle revolution, mm/rev, always positive.
struct Pitches {
    double x{};
    double z{};
};

/// The leadscrew-related HAL outputs. Pin names in comments are the `lathe.*` HAL pins.
struct LeadscrewOutputs {
    bool cycle_control{};      ///< control_source: motion controller drives the steppers
    double z_feed{};           ///< forward_z: signed mm per spindle revolution
    double x_feed{};           ///< forward_x
    bool z_driver_enabled{};   ///< enable_z: stepper driver powered (axis locked)
    bool x_driver_enabled{};   ///< enable_x
    bool z_follows_spindle{};  ///< enable_stepper_z: step generator engaged
    bool x_follows_spindle{};  ///< enable_stepper_x
};

/// Manual mode: the electronic leadscrew as selected on the operator panel.
[[nodiscard]] auto manual_leadscrew(FeedMode feed, DirectionMode direction, const Pitches& pitches)
    -> LeadscrewOutputs;

/// Canned-cycle mode: LinuxCNC's motion controller owns both axes, drivers powered.
[[nodiscard]] auto cycle_leadscrew(const Pitches& pitches) -> LeadscrewOutputs;

} // namespace elle::core

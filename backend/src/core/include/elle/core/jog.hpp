#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace elle::core {

/// The four jog buttons. On the panel "up" retracts the cross slide (X-) and "left" moves towards
/// the chuck (Z-).
enum class JogButton : std::uint8_t {
    x_minus,
    x_plus,
    z_minus,
    z_plus,
};

/// Commanded slide velocity in mm/s for one axis; empty means the axis is not being jogged and is
/// back under position control.
struct JogVelocities {
    std::optional<double> x{};
    std::optional<double> z{};
};

/// Jog state machine with a dead-man timeout.
///
/// Velocity ramps up linearly with the time a button has been held, as in the original panel. A
/// held button must be refreshed (`hold`) at least every `deadman_timeout`; otherwise the jog stops
/// on its own, so a frozen or disconnected user interface can never leave a slide running.
class Jog {
public:
    using Clock = std::chrono::steady_clock;

    static constexpr auto deadman_timeout = std::chrono::milliseconds{150};

    /// Button went down, or is still down. Starts the ramp on the first call.
    auto hold(JogButton button, Clock::time_point now) -> void;
    /// Button went up.
    auto release(JogButton button) -> void;
    /// Every button up.
    auto release_all() -> void;

    /// Velocities to command at `now`. Expires buttons whose refresh is overdue.
    [[nodiscard]] auto velocities(Clock::time_point now) -> JogVelocities;

private:
    struct Held {
        Clock::time_point since;
        Clock::time_point refreshed;
    };
    static constexpr std::size_t button_count = 4;
    std::array<std::optional<Held>, button_count> held_{};
};

} // namespace elle::core

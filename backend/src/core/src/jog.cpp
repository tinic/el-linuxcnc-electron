#include <elle/core/jog.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <optional>
#include <utility>

namespace elle::core {

namespace {

/// mm/s gained per second of holding a button.
constexpr double ramp_per_second = 3.0;
constexpr double x_max_speed = 3.0;
constexpr double z_max_speed = 6.0;

auto index_of(JogButton button) -> std::size_t
{
    return static_cast<std::size_t>(std::to_underlying(button));
}

auto speed(Jog::Clock::time_point since, Jog::Clock::time_point now, double max_speed) -> double
{
    const std::chrono::duration<double> held = now - since;
    return std::min(held.count() * ramp_per_second, max_speed);
}

} // namespace

auto Jog::hold(JogButton button, Clock::time_point now) -> void
{
    auto& slot = held_.at(index_of(button));
    if (slot.has_value()) {
        slot->refreshed = now;
    } else {
        slot = Held{.since = now, .refreshed = now};
    }
}

auto Jog::release(JogButton button) -> void
{
    held_.at(index_of(button)).reset();
}

auto Jog::release_all() -> void
{
    for (auto& slot : held_) {
        slot.reset();
    }
}

auto Jog::velocities(Clock::time_point now) -> JogVelocities
{
    for (auto& slot : held_) {
        if (slot.has_value() && now - slot->refreshed > deadman_timeout) {
            slot.reset();
        }
    }

    const auto& x_minus = held_.at(index_of(JogButton::x_minus));
    const auto& x_plus = held_.at(index_of(JogButton::x_plus));
    const auto& z_minus = held_.at(index_of(JogButton::z_minus));
    const auto& z_plus = held_.at(index_of(JogButton::z_plus));

    // Opposing buttons on one axis cancel out: the slide stops rather than guessing.
    JogVelocities result{};
    if (x_minus.has_value() != x_plus.has_value()) {
        result.x = x_minus.has_value() ? -speed(x_minus->since, now, x_max_speed)
                                       : speed(x_plus->since, now, x_max_speed);
    }
    if (z_minus.has_value() != z_plus.has_value()) {
        result.z = z_minus.has_value() ? -speed(z_minus->since, now, z_max_speed)
                                       : speed(z_plus->since, now, z_max_speed);
    }
    return result;
}

} // namespace elle::core

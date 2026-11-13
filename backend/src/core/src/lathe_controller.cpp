#include <elle/core/lathe_controller.hpp>

#include <elle/core/cycle.hpp>
#include <elle/core/jog.hpp>
#include <elle/core/leadscrew.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <utility>
#include <vector>

namespace elle::core {

namespace {

constexpr double degrees_per_revolution = 360.0;
constexpr double seconds_per_minute = 60.0;

/// Ticks to wait after commanding an encoder reset before trusting the positions again. The encoder
/// component polls its reset pins every millisecond and resets the device before its next position
/// read, so 20-30 ms leaves a wide margin.
constexpr int reset_settle_ticks = 3;

/// The original smoothed the speed by 0.2 per 1/30 s poll. Expressed per elapsed time so the
/// filter behaves the same at any tick rate.
constexpr double rpm_retained_per_reference_step = 0.8;
constexpr std::chrono::duration<double> rpm_reference_step{1.0 / 30.0};

/// Fractional part in [0, 1): where in the revolution the spindle is, whatever the sign.
auto revolution_fraction(double revolutions) -> double
{
    const double fraction = revolutions - std::floor(revolutions);
    return fraction >= 1.0 ? 0.0 : fraction;
}

} // namespace

LatheController::LatheController(MachineSettings settings)
    : settings_{std::move(settings)}
{
}

auto LatheController::select_mode(OperatingMode mode) -> void
{
    if (mode == mode_) {
        return;
    }
    mode_ = mode;
    reset_requested_ = true;
    if (mode_ != OperatingMode::manual) {
        jog_.release_all();
    }
    if (mode_ == OperatingMode::cycle) {
        // Motion drives the slides during a cycle: both powered and holding, nothing following
        // the spindle.
        feed_ = FeedMode::back_compound;
        direction_ = DirectionMode::hold;
        back_from_cycle_ = true;
    } else if (mode_ == OperatingMode::manual && back_from_cycle_) {
        // Back from canned cycles (directly or via another page): the start-up state,
        // longitudinal feed with the drivers off.
        feed_ = FeedMode::longitudinal;
        direction_ = DirectionMode::idle;
        back_from_cycle_ = false;
    }
    update_leadscrew();
}

auto LatheController::set_feed(FeedMode feed) -> void
{
    feed_ = feed;
    update_leadscrew();
}

auto LatheController::set_direction(DirectionMode direction) -> void
{
    direction_ = direction;
    update_leadscrew();
}

auto LatheController::set_pitches(const Pitches& pitches) -> void
{
    settings_.pitches = {.x = std::abs(pitches.x), .z = std::abs(pitches.z)};
    update_leadscrew();
}

auto LatheController::set_encoder_scales(double scale_z, double scale_x) -> void
{
    settings_.encoder_scale_z = scale_z;
    settings_.encoder_scale_x = scale_x;
}

auto LatheController::set_axis(Axis axis, double value) -> void
{
    switch (axis) {
    case Axis::x:
        offset_x_ = sample_.position_x + tool_offset_x() - value;
        break;
    case Axis::z:
        offset_z_ = sample_.position_z + tool_offset_z() - value;
        break;
    case Axis::a:
    default:
        offset_a_ = sample_.spindle_revs - revolution_fraction(value / degrees_per_revolution);
        break;
    }
    relatch_requested_ = true;
}

auto LatheController::set_tools(std::vector<Tool> tools) -> void
{
    settings_.tools = std::move(tools);
}

auto LatheController::select_tool(int tool_id) -> void
{
    settings_.current_tool = tool_id;
}

auto LatheController::jog_hold(JogButton button, Clock::time_point now) -> void
{
    if (mode_ == OperatingMode::manual) {
        jog_.hold(button, now);
    }
}

auto LatheController::jog_release(JogButton button) -> void
{
    jog_.release(button);
}

auto LatheController::jog_release_all() -> void
{
    jog_.release_all();
}

auto LatheController::tick(const MachineSample& sample, Clock::time_point now) -> TickResult
{
    TickResult result{};
    if (reset_requested_) {
        reset_requested_ = false;
        pending_reset_ = PendingReset{.position_z = sample.position_z,
                                      .position_x = sample.position_x,
                                      .ticks_left = reset_settle_ticks};
        ++outputs_.reset_z;
        ++outputs_.reset_x;
        result.resync_motion = true;
    }
    const bool positions_valid = advance_reset();

    // While the encoders are being zeroed the readout holds its last value instead of flickering.
    sample_.spindle_revs = sample.spindle_revs;
    sample_.spindle_rps = sample.spindle_rps;
    sample_.stepper_z = sample.stepper_z;
    sample_.stepper_x = sample.stepper_x;
    if (positions_valid) {
        sample_.position_z = sample.position_z;
        sample_.position_x = sample.position_x;
    }
    smooth_rpm(sample, now);

    apply_jog(jog_.velocities(now));

    outputs_.control_source = leadscrew_.cycle_control;
    outputs_.forward_z = leadscrew_.z_feed;
    outputs_.forward_x = leadscrew_.x_feed;
    outputs_.enable_z = leadscrew_.z_driver_enabled;
    outputs_.enable_x = leadscrew_.x_driver_enabled;
    outputs_.enable_stepper_z = leadscrew_.z_follows_spindle;
    outputs_.enable_stepper_x = leadscrew_.x_follows_spindle;
    outputs_.scale_encoder_z = settings_.encoder_scale_z;
    outputs_.scale_encoder_x = settings_.encoder_scale_x;
    if (relatch_requested_) {
        relatch_requested_ = false;
        relatch(sample);
    }

    result.outputs = outputs_;
    return result;
}

auto LatheController::readout() const -> Readout
{
    return {.x = sample_.position_x - offset_x_ + tool_offset_x(),
            .z = sample_.position_z - offset_z_ + tool_offset_z(),
            .a_degrees = revolution_fraction(sample_.spindle_revs - offset_a_) *
                         degrees_per_revolution,
            .rpm = rpm_};
}

auto LatheController::work_position() const -> WorkPosition
{
    const Readout current = readout();
    return {.x = current.x, .z = current.z, .a = sample_.spindle_revs - offset_a_};
}

auto LatheController::tool_offset_x() const -> double
{
    const auto tool = std::ranges::find(settings_.tools, settings_.current_tool, &Tool::id);
    return tool == settings_.tools.end() ? 0.0 : tool->offset_x;
}

auto LatheController::tool_offset_z() const -> double
{
    const auto tool = std::ranges::find(settings_.tools, settings_.current_tool, &Tool::id);
    return tool == settings_.tools.end() ? 0.0 : tool->offset_z;
}

auto LatheController::update_leadscrew() -> void
{
    switch (mode_) {
    case OperatingMode::manual:
        leadscrew_ = manual_leadscrew(feed_, direction_, settings_.pitches);
        break;
    case OperatingMode::cycle:
        leadscrew_ = cycle_leadscrew(settings_.pitches);
        break;
    case OperatingMode::other:
    default:
        return;
    }
    relatch_requested_ = true;
}

/// Re-references the spindle-to-stepper gearing to "now": the position chain in lathe.hal
/// computes stepper = stepper_at_latch + feed * (revs - revs_at_latch), so latching whenever the
/// gearing changes (or a jog moved the stepper) avoids any jump.
auto LatheController::relatch(const MachineSample& sample) -> void
{
    outputs_.offset_z_encoder = -sample.spindle_revs;
    outputs_.offset_z_stepper = sample.stepper_z;
    outputs_.offset_x_encoder = -sample.spindle_revs;
    outputs_.offset_x_stepper = sample.stepper_x;
}

auto LatheController::apply_jog(const JogVelocities& velocities) -> void
{
    const bool jog_x = velocities.x.has_value();
    const bool jog_z = velocities.z.has_value();
    outputs_.control_x_type = jog_x;
    outputs_.velocity_x_cmd = velocities.x.value_or(0.0);
    outputs_.control_z_type = jog_z;
    outputs_.velocity_z_cmd = velocities.z.value_or(0.0);
    // A jog moves the stepper outside the position chain; re-latch while and after jogging.
    if (jog_x || jog_z || jogging_x_ || jogging_z_) {
        relatch_requested_ = true;
    }
    jogging_x_ = jog_x;
    jogging_z_ = jog_z;
}

/// Counts down an in-flight encoder reset. Returns whether this tick's encoder positions can be
/// used, i.e. no reset is settling.
auto LatheController::advance_reset() -> bool
{
    if (!pending_reset_.has_value()) {
        return true;
    }
    if (--pending_reset_->ticks_left > 0) {
        return false;
    }
    // The encoders now count from zero. Moving the offsets by the pre-reset positions keeps every
    // readout exactly where it was (machine - offset is unchanged).
    offset_z_ -= pending_reset_->position_z;
    offset_x_ -= pending_reset_->position_x;
    pending_reset_.reset();
    return true;
}

auto LatheController::smooth_rpm(const MachineSample& sample, Clock::time_point now) -> void
{
    const double rpm = std::abs(sample.spindle_rps * seconds_per_minute);
    if (last_tick_.has_value()) {
        const std::chrono::duration<double> elapsed = now - *last_tick_;
        const double retained =
            std::pow(rpm_retained_per_reference_step, elapsed / rpm_reference_step);
        rpm_ = (rpm_ * retained) + (rpm * (1.0 - retained));
    }
    last_tick_ = now;
}

} // namespace elle::core

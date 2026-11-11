#include <elle/core/leadscrew.hpp>

namespace elle::core {

namespace {

/// One row of the original engagement table (useHAL.ts updateHALOut).
struct Engagement {
    bool z_follows{};
    bool x_follows{};
    bool z_powered{};
    bool x_powered{};
    bool z_forward{};
    bool x_forward{};
};

constexpr Engagement idle_z_forward{.z_follows = false, .x_follows = false, .z_powered = false,
                                    .x_powered = false, .z_forward = true, .x_forward = true};

auto longitudinal(DirectionMode direction) -> Engagement
{
    switch (direction) {
    case DirectionMode::forward:
        return {.z_follows = true, .x_follows = false, .z_powered = true, .x_powered = false,
                .z_forward = true, .x_forward = true};
    case DirectionMode::reverse:
        return {.z_follows = true, .x_follows = false, .z_powered = true, .x_powered = false,
                .z_forward = false, .x_forward = false};
    case DirectionMode::hold:
        return {.z_follows = false, .x_follows = false, .z_powered = true, .x_powered = false,
                .z_forward = true, .x_forward = true};
    case DirectionMode::idle:
    default:
        return idle_z_forward;
    }
}

auto cross(DirectionMode direction) -> Engagement
{
    switch (direction) {
    case DirectionMode::forward:
        return {.z_follows = false, .x_follows = true, .z_powered = false, .x_powered = true,
                .z_forward = true, .x_forward = false};
    case DirectionMode::reverse:
        return {.z_follows = false, .x_follows = true, .z_powered = false, .x_powered = true,
                .z_forward = false, .x_forward = true};
    case DirectionMode::hold:
        return {.z_follows = false, .x_follows = false, .z_powered = false, .x_powered = true,
                .z_forward = true, .x_forward = true};
    case DirectionMode::idle:
    default:
        return idle_z_forward;
    }
}

auto front_compound(DirectionMode direction) -> Engagement
{
    switch (direction) {
    case DirectionMode::forward:
        return {.z_follows = true, .x_follows = true, .z_powered = true, .x_powered = true,
                .z_forward = true, .x_forward = true};
    case DirectionMode::reverse:
        return {.z_follows = true, .x_follows = true, .z_powered = true, .x_powered = true,
                .z_forward = false, .x_forward = false};
    case DirectionMode::hold:
        return {.z_follows = false, .x_follows = false, .z_powered = true, .x_powered = true,
                .z_forward = true, .x_forward = true};
    case DirectionMode::idle:
    default:
        return idle_z_forward;
    }
}

auto back_compound(DirectionMode direction) -> Engagement
{
    switch (direction) {
    case DirectionMode::forward:
        return {.z_follows = true, .x_follows = true, .z_powered = true, .x_powered = true,
                .z_forward = true, .x_forward = false};
    case DirectionMode::reverse:
        return {.z_follows = true, .x_follows = true, .z_powered = true, .x_powered = true,
                .z_forward = false, .x_forward = true};
    case DirectionMode::hold:
        return {.z_follows = false, .x_follows = false, .z_powered = true, .x_powered = true,
                .z_forward = true, .x_forward = false};
    case DirectionMode::idle:
    default:
        return {.z_follows = false, .x_follows = false, .z_powered = false, .x_powered = false,
                .z_forward = true, .x_forward = false};
    }
}

auto engagement(FeedMode feed, DirectionMode direction) -> Engagement
{
    switch (feed) {
    case FeedMode::longitudinal:
        return longitudinal(direction);
    case FeedMode::cross:
        return cross(direction);
    case FeedMode::front_compound:
        return front_compound(direction);
    case FeedMode::back_compound:
    default:
        return back_compound(direction);
    }
}

/// Z "forward" moves towards the chuck (negative Z); X "forward" moves away from the axis.
auto signed_feeds(const Engagement& row, const Pitches& pitches) -> Pitches
{
    return {.x = row.x_forward ? pitches.x : -pitches.x, .z = row.z_forward ? -pitches.z : pitches.z};
}

} // namespace

auto manual_leadscrew(FeedMode feed, DirectionMode direction, const Pitches& pitches)
    -> LeadscrewOutputs
{
    const Engagement row = engagement(feed, direction);
    const Pitches feeds = signed_feeds(row, pitches);
    return {.cycle_control = false,
            .z_feed = feeds.z,
            .x_feed = feeds.x,
            .z_driver_enabled = row.z_powered,
            .x_driver_enabled = row.x_powered,
            .z_follows_spindle = row.z_follows,
            .x_follows_spindle = row.x_follows};
}

auto cycle_leadscrew(const Pitches& pitches) -> LeadscrewOutputs
{
    // The original switched the panel to back compound + hold when entering canned cycles; the
    // feeds only matter for the manual position chain, which the motion controller bypasses.
    const Pitches feeds = signed_feeds(back_compound(DirectionMode::hold), pitches);
    return {.cycle_control = true,
            .z_feed = feeds.z,
            .x_feed = feeds.x,
            .z_driver_enabled = true,
            .x_driver_enabled = true,
            .z_follows_spindle = true,
            .x_follows_spindle = true};
}

} // namespace elle::core

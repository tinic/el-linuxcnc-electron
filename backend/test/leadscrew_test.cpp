#include <elle/core/leadscrew.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <format>
#include <string>
#include <string_view>

using elle::core::DirectionMode;
using elle::core::FeedMode;
using elle::core::LeadscrewOutputs;
using elle::core::Pitches;

namespace {

constexpr Pitches pitches{.x = 0.02, .z = 0.05};

/// Compact rendering of the outputs: follows/powered per axis and signed feeds.
auto render(const LeadscrewOutputs& out) -> std::string
{
    return std::format("cycle={} z[follow={} power={} feed={}] x[follow={} power={} feed={}]",
                       out.cycle_control, out.z_follows_spindle, out.z_driver_enabled,
                       out.z_feed, out.x_follows_spindle, out.x_driver_enabled, out.x_feed);
}

struct Row {
    FeedMode feed{};
    DirectionMode direction{};
    std::string_view expected{};
};

// The original engagement table from useHAL.ts updateHALOut, transcribed row by row. Feeds follow
// the original sign rule: forward_z = zforward ? -pz : +pz, forward_x = xforward ? +px : -px.
constexpr std::array table{
    Row{.feed = FeedMode::longitudinal,
        .direction = DirectionMode::forward,
        .expected = "cycle=false z[follow=true power=true feed=-0.05] x[follow=false power=false feed=0.02]"},
    Row{.feed = FeedMode::longitudinal,
        .direction = DirectionMode::reverse,
        .expected = "cycle=false z[follow=true power=true feed=0.05] x[follow=false power=false feed=-0.02]"},
    Row{.feed = FeedMode::longitudinal,
        .direction = DirectionMode::hold,
        .expected = "cycle=false z[follow=false power=true feed=-0.05] x[follow=false power=false feed=0.02]"},
    Row{.feed = FeedMode::longitudinal,
        .direction = DirectionMode::idle,
        .expected = "cycle=false z[follow=false power=false feed=-0.05] x[follow=false power=false feed=0.02]"},
    Row{.feed = FeedMode::cross,
        .direction = DirectionMode::forward,
        .expected = "cycle=false z[follow=false power=false feed=-0.05] x[follow=true power=true feed=-0.02]"},
    Row{.feed = FeedMode::cross,
        .direction = DirectionMode::reverse,
        .expected = "cycle=false z[follow=false power=false feed=0.05] x[follow=true power=true feed=0.02]"},
    Row{.feed = FeedMode::cross,
        .direction = DirectionMode::hold,
        .expected = "cycle=false z[follow=false power=false feed=-0.05] x[follow=false power=true feed=0.02]"},
    Row{.feed = FeedMode::cross,
        .direction = DirectionMode::idle,
        .expected = "cycle=false z[follow=false power=false feed=-0.05] x[follow=false power=false feed=0.02]"},
    Row{.feed = FeedMode::front_compound,
        .direction = DirectionMode::forward,
        .expected = "cycle=false z[follow=true power=true feed=-0.05] x[follow=true power=true feed=0.02]"},
    Row{.feed = FeedMode::front_compound,
        .direction = DirectionMode::reverse,
        .expected = "cycle=false z[follow=true power=true feed=0.05] x[follow=true power=true feed=-0.02]"},
    Row{.feed = FeedMode::front_compound,
        .direction = DirectionMode::hold,
        .expected = "cycle=false z[follow=false power=true feed=-0.05] x[follow=false power=true feed=0.02]"},
    Row{.feed = FeedMode::front_compound,
        .direction = DirectionMode::idle,
        .expected = "cycle=false z[follow=false power=false feed=-0.05] x[follow=false power=false feed=0.02]"},
    Row{.feed = FeedMode::back_compound,
        .direction = DirectionMode::forward,
        .expected = "cycle=false z[follow=true power=true feed=-0.05] x[follow=true power=true feed=-0.02]"},
    Row{.feed = FeedMode::back_compound,
        .direction = DirectionMode::reverse,
        .expected = "cycle=false z[follow=true power=true feed=0.05] x[follow=true power=true feed=0.02]"},
    Row{.feed = FeedMode::back_compound,
        .direction = DirectionMode::hold,
        .expected = "cycle=false z[follow=false power=true feed=-0.05] x[follow=false power=true feed=-0.02]"},
    Row{.feed = FeedMode::back_compound,
        .direction = DirectionMode::idle,
        .expected = "cycle=false z[follow=false power=false feed=-0.05] x[follow=false power=false feed=-0.02]"},
};

} // namespace

TEST_CASE("manual leadscrew follows the original engagement table", "[leadscrew]")
{
    for (const Row& row : table) {
        CHECK(render(elle::core::manual_leadscrew(row.feed, row.direction, pitches)) == row.expected);
    }
}

TEST_CASE("canned cycles hand both axes to the motion controller", "[leadscrew]")
{
    CHECK(render(elle::core::cycle_leadscrew(pitches)) ==
          "cycle=true z[follow=true power=true feed=-0.05] x[follow=true power=true feed=-0.02]");
}

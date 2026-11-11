#include <elle/core/backplot.hpp>
#include <elle/core/cycle.hpp>
#include <elle/core/threading.hpp>
#include <elle/core/turning.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstddef>
#include <string>

using Catch::Approx;
using elle::core::Destination;
using elle::core::MoveKind;

TEST_CASE("a turning preview plots every pass", "[backplot]")
{
    const auto program =
        elle::core::plan_turning({.target = 4.0, .z_end = -20.0}, {.x = 5.3, .z = 0.0, .a = 0.0});
    const auto gcode = elle::core::turning_gcode(program, Destination::preview);
    REQUIRE(gcode.has_value());
    const auto plot = elle::core::backplot(*gcode, {.x = 5.3, .z = 0.0});
    INFO((plot.has_value() ? std::string{} : plot.error().message));
    REQUIRE(plot.has_value());

    const auto threads = std::ranges::count(plot->segments, MoveKind::thread,
                                            &elle::core::Segment::kind);
    CHECK(threads == 4); // three roughing passes and the final pass
    CHECK(plot->bounds.min.z == Approx(-20.0));
    CHECK(plot->bounds.max.z == Approx(2.0));
    CHECK(plot->bounds.min.x == Approx(4.0));
    CHECK(plot->segments.back().to.x == Approx(4.0));
}

TEST_CASE("the machine variant of a thread plots the same path as the preview", "[backplot]")
{
    const auto program = elle::core::plan_threading(
        {.pitch = 1.0, .x_depth = -0.613435, .z_end = -6.0}, {.x = 3.0, .z = 0.0, .a = 0.0});
    const auto preview = elle::core::threading_gcode(program, Destination::preview);
    const auto machine = elle::core::threading_gcode(program, Destination::machine);
    REQUIRE(preview.has_value());
    REQUIRE(machine.has_value());
    const auto preview_plot = elle::core::backplot(*preview, {.x = 3.0, .z = 0.0});
    const auto machine_plot = elle::core::backplot(*machine, {.x = 3.0, .z = 0.0});
    REQUIRE(preview_plot.has_value());
    REQUIRE(machine_plot.has_value());
    REQUIRE(preview_plot->segments.size() == machine_plot->segments.size());
    for (std::size_t index = 0; index < preview_plot->segments.size(); ++index) {
        CHECK(preview_plot->segments.at(index).to.x ==
              Approx(machine_plot->segments.at(index).to.x));
        CHECK(preview_plot->segments.at(index).to.z ==
              Approx(machine_plot->segments.at(index).to.z));
    }
}

TEST_CASE("unknown G-code is reported, not skipped", "[backplot]")
{
    const auto plot = elle::core::backplot({"G0 X1 Z2", "G2 X0 Z0 R1"}, {});
    REQUIRE(!plot.has_value());
    CHECK(plot.error().line == 1);
}

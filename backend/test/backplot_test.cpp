#include <elle/core/backplot.hpp>
#include <elle/core/cycle.hpp>
#include <elle/core/threading.hpp>
#include <elle/core/turning.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstddef>
#include <expected>
#include <utility>

using Catch::Approx;
using elle::core::Destination;
using elle::core::MoveKind;

namespace {

/// Plots a program, or an empty plot (with the reason logged) if it cannot be generated.
auto plot_of(const std::expected<elle::core::GcodeLines, elle::core::CycleError>& gcode,
             elle::core::PlotPoint start) -> elle::core::Backplot
{
    if (!gcode.has_value()) {
        FAIL_CHECK(elle::core::describe(gcode.error()));
        return {};
    }
    auto plot = elle::core::backplot(*gcode, start);
    if (!plot.has_value()) {
        FAIL_CHECK(plot.error().message);
        return {};
    }
    return std::move(*plot);
}

auto turning_plot() -> elle::core::Backplot
{
    const auto program =
        elle::core::plan_turning({.target = 4.0, .z_end = -20.0}, {.x = 5.3, .z = 0.0, .a = 0.0});
    return plot_of(elle::core::turning_gcode(program, Destination::preview), {.x = 5.3, .z = 0.0});
}

auto thread_plot(Destination destination) -> elle::core::Backplot
{
    const auto program = elle::core::plan_threading(
        {.pitch = 1.0, .x_depth = -0.613435, .z_end = -6.0}, {.x = 3.0, .z = 0.0, .a = 0.0});
    return plot_of(elle::core::threading_gcode(program, destination), {.x = 3.0, .z = 0.0});
}

} // namespace

TEST_CASE("a turning preview plots every pass", "[backplot]")
{
    const auto plot = turning_plot();
    // three roughing passes and the final pass
    CHECK(std::ranges::count(plot.segments, MoveKind::thread, &elle::core::Segment::kind) == 4);
}

TEST_CASE("a turning preview spans the cut", "[backplot]")
{
    const auto plot = turning_plot();
    CHECK(plot.bounds.min.z == Approx(-20.0));
    CHECK(plot.bounds.max.z == Approx(2.0));
    CHECK(plot.bounds.min.x == Approx(4.0));
    REQUIRE(!plot.segments.empty());
    CHECK(plot.segments.back().to.x == Approx(4.0));
}

TEST_CASE("the machine variant of a thread plots the same path as the preview", "[backplot]")
{
    const auto preview = thread_plot(Destination::preview);
    const auto machine = thread_plot(Destination::machine);
    REQUIRE(preview.segments.size() == machine.segments.size());
    for (std::size_t index = 0; index < preview.segments.size(); ++index) {
        CHECK(preview.segments.at(index).to.x == Approx(machine.segments.at(index).to.x));
        CHECK(preview.segments.at(index).to.z == Approx(machine.segments.at(index).to.z));
    }
}

TEST_CASE("unknown G-code is reported, not skipped", "[backplot]")
{
    const auto plot = elle::core::backplot({"G0 X1 Z2", "G2 X0 Z0 R1"}, {});
    REQUIRE(!plot.has_value());
    CHECK(plot.error().line == 1);
}

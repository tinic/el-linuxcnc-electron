// Inputs on which the reference implementation hung or crashed must produce an error instead.

#include <elle/core/cycle.hpp>
#include <elle/core/threading.hpp>
#include <elle/core/turning.hpp>

#include <catch2/catch_test_macros.hpp>

#include <limits>

using elle::core::CycleError;
using elle::core::Destination;

namespace {

auto valid_thread() -> elle::core::ThreadingProgram
{
    return elle::core::plan_threading({.pitch = 1.0, .x_depth = -0.6, .z_end = -10.0}, {});
}

auto valid_turning() -> elle::core::TurningProgram
{
    return elle::core::plan_turning({.target = 4.0, .z_end = -20.0}, {.x = 5.0, .z = 0.0, .a = 0.0});
}

} // namespace

TEST_CASE("a valid thread generates", "[errors][threading]")
{
    CHECK(elle::core::threading_gcode(valid_thread(), Destination::machine).has_value());
}

TEST_CASE("non-finite threading parameters are rejected", "[errors][threading]")
{
    auto program = valid_thread();
    program.x_end = std::numeric_limits<double>::infinity();
    const auto result = elle::core::threading_gcode(program, Destination::preview);
    REQUIRE(!result.has_value());
    CHECK(result.error() == CycleError::non_finite_parameter);
}

TEST_CASE("a thread that would need an absurd number of passes is rejected", "[errors][threading]")
{
    // 0.1 + 0.05 + 0.025 + ... converges to 0.2 mm; the remaining 0.4 mm of the 0.6 mm depth would
    // then take 400 million passes of the 1 nm minimum cut.
    auto program = valid_thread();
    program.first_cut = 0.1;
    program.cut_mult = 0.5;
    program.min_cut = 1e-9;
    const auto result = elle::core::threading_gcode(program, Destination::machine);
    REQUIRE(!result.has_value());
    CHECK(result.error() == CycleError::too_many_passes);
}

TEST_CASE("a valid turning cycle generates", "[errors][turning]")
{
    CHECK(elle::core::turning_gcode(valid_turning(), Destination::machine).has_value());
}

TEST_CASE("zero step down with material left to rough is rejected", "[errors][turning]")
{
    auto program = valid_turning();
    program.step_down = 0.0;
    const auto result = elle::core::turning_gcode(program, Destination::machine);
    REQUIRE(!result.has_value());
    CHECK(result.error() == CycleError::zero_step_down);
}

TEST_CASE("a step down too small to finish is rejected", "[errors][turning]")
{
    auto program = valid_turning();
    program.step_down = 1e-9;
    const auto result = elle::core::turning_gcode(program, Destination::machine);
    REQUIRE(!result.has_value());
    CHECK(result.error() == CycleError::too_many_passes);
}

TEST_CASE("every cycle error has a description", "[errors]")
{
    for (const CycleError error : {CycleError::non_finite_parameter, CycleError::too_many_passes,
                                   CycleError::zero_step_down}) {
        CHECK(!elle::core::describe(error).empty());
    }
}

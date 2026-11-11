#include <elle/core/numbers.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cmath>

using elle::core::exactly_equal;
using elle::core::format_coordinate;
using elle::core::round_for_linuxcnc;
using elle::core::round_half_up;

TEST_CASE("round_for_linuxcnc keeps six decimals", "[numbers]")
{
    CHECK(exactly_equal(round_for_linuxcnc(1.0000004), 1.0));
    CHECK(exactly_equal(round_for_linuxcnc(1.0000006), 1.000001));
    CHECK(exactly_equal(round_for_linuxcnc(-0.6134354), -0.613435));
    CHECK(exactly_equal(round_for_linuxcnc(-0.6134356), -0.613436));
}

TEST_CASE("round_for_linuxcnc never yields negative zero", "[numbers]")
{
    CHECK(!std::signbit(round_for_linuxcnc(-0.0)));
    CHECK(!std::signbit(round_for_linuxcnc(-1e-9)));
}

TEST_CASE("round_half_up rounds halves towards positive infinity", "[numbers]")
{
    CHECK(exactly_equal(round_half_up(2.5), 3.0));
    CHECK(exactly_equal(round_half_up(-2.5), -2.0));
    CHECK(exactly_equal(round_half_up(1.4999999), 1.0));
    CHECK(exactly_equal(round_half_up(0.49999999999999994), 0.0));
}

TEST_CASE("format_coordinate matches Python's .6f formatting", "[numbers]")
{
    CHECK(format_coordinate(0.0) == "0.000000");
    CHECK(format_coordinate(-0.0) == "-0.000000");
    CHECK(format_coordinate(-0.6134355) == "-0.613436");
    CHECK(format_coordinate(1234.5) == "1234.500000");
}

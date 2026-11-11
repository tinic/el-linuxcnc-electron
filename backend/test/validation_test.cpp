#include <elle/core/cycle.hpp>
#include <elle/core/threading.hpp>
#include <elle/core/turning.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <limits>
#include <string_view>

using elle::core::ParameterIssues;

namespace {

auto flags(const ParameterIssues& issues, std::string_view field) -> bool
{
    return std::ranges::any_of(issues, [field](const auto& issue) { return issue.field == field; });
}

constexpr elle::core::ThreadingRequest m6{.pitch = 1.0, .x_depth = -0.613435, .z_end = -6.0};
constexpr elle::core::TurningRequest to_8mm{.target = 4.0, .z_end = -20.0};
constexpr elle::core::WorkPosition stock_10mm{.x = 5.0, .z = 0.0, .a = 0.0};

} // namespace

TEST_CASE("a complete thread passes validation", "[validation][threading]")
{
    CHECK(elle::core::validate_threading(m6).empty());
}

TEST_CASE("threading requires pitch and Z end", "[validation][threading]")
{
    const auto issues = elle::core::validate_threading({});
    CHECK(flags(issues, "pitch"));
    CHECK(flags(issues, "z_end"));
}

TEST_CASE("threading rejects out-of-range parameters", "[validation][threading]")
{
    auto request = m6;
    request.pitch = 0.0;
    request.cut_mult = 1.5;
    request.min_cut = 0.0;
    request.spring_cuts = 11.0;
    request.x_depth = std::numeric_limits<double>::quiet_NaN();
    const auto issues = elle::core::validate_threading(request);
    CHECK(flags(issues, "pitch"));
    CHECK(flags(issues, "cut_mult"));
    CHECK(flags(issues, "min_cut"));
    CHECK(flags(issues, "spring_cuts"));
    CHECK(flags(issues, "x_depth"));
}

TEST_CASE("zero spring cuts is a valid entry", "[validation][threading]")
{
    auto request = m6;
    request.spring_cuts = 0.0;
    CHECK(elle::core::validate_threading(request).empty());
    CHECK(elle::core::plan_threading(request, {}).spring_cuts == 0);
}

TEST_CASE("a complete turning cycle passes validation", "[validation][turning]")
{
    CHECK(elle::core::validate_turning(to_8mm, stock_10mm).empty());
}

TEST_CASE("turning refuses a target at or above the stock", "[validation][turning]")
{
    CHECK(flags(elle::core::validate_turning(to_8mm, {.x = 4.0, .z = 0.0, .a = 0.0}), "target"));
    CHECK(flags(elle::core::validate_turning(to_8mm, {.x = 3.0, .z = 0.0, .a = 0.0}), "target"));
}

TEST_CASE("turning rejects out-of-range parameters", "[validation][turning]")
{
    auto request = to_8mm;
    request.z_end = 1.0;
    request.step_down = 0.0;
    request.taper_angle_deg = 60.0;
    const auto issues = elle::core::validate_turning(request, stock_10mm);
    CHECK(flags(issues, "z_end"));
    CHECK(flags(issues, "step_down"));
    CHECK(flags(issues, "taper_angle_deg"));
}

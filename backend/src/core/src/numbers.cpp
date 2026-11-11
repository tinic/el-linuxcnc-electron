#include <elle/core/numbers.hpp>

#include <charconv>
#include <cmath>
#include <format>
#include <memory>
#include <numbers>
#include <string>
#include <system_error>

namespace elle::core {

namespace {

constexpr double half = 0.5;
constexpr double degrees_to_radians = std::numbers::pi / 180.0;

} // namespace

auto round_for_linuxcnc(double value) -> double
{
    const std::string text = format_coordinate(value);
    double parsed{};
    const auto result = std::from_chars(text.data(), std::to_address(text.cend()), parsed);
    if (result.ec != std::errc{}) {
        return value;
    }
    // JSON.stringify(-0) is "0", so the reference backend never saw a negative zero.
    return exactly_equal(parsed, 0.0) ? 0.0 : parsed;
}

auto round_half_up(double value) -> double
{
    const double floor_value = std::floor(value);
    return (value - floor_value >= half) ? floor_value + 1.0 : floor_value;
}

auto exactly_equal(double lhs, double rhs) noexcept -> bool
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
    return lhs == rhs;
#pragma GCC diagnostic pop
}

auto format_coordinate(double value) -> std::string
{
    return std::format("{:.6f}", value);
}

auto radians(double degrees) noexcept -> double
{
    return degrees * degrees_to_radians;
}

} // namespace elle::core

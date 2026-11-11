#include "gcode_common.hpp"

#include <elle/core/cycle.hpp>
#include <elle/core/numbers.hpp>

#include <cmath>
#include <format>
#include <limits>
#include <string_view>

namespace elle::core::detail {

auto append_preamble(GcodeLines& lines) -> void
{
    lines.emplace_back("G8");
    lines.emplace_back("G21");
    lines.emplace_back("G90");
    lines.emplace_back("F100");
    lines.emplace_back("M3S500");
}

auto append_work_offset(GcodeLines& lines, const WorkPosition& position) -> void
{
    lines.push_back(std::format("G10 L20 P1 X{} Z{}", format_coordinate(position.x),
                                format_coordinate(position.z)));
    lines.emplace_back("G54");
}

auto saturating_count(double rounded) noexcept -> int
{
    constexpr auto lowest = static_cast<double>(std::numeric_limits<int>::lowest());
    constexpr auto highest = static_cast<double>(std::numeric_limits<int>::max());
    if (std::isnan(rounded)) {
        return 0;
    }
    if (rounded <= lowest) {
        return std::numeric_limits<int>::lowest();
    }
    if (rounded >= highest) {
        return std::numeric_limits<int>::max();
    }
    return static_cast<int>(rounded);
}

} // namespace elle::core::detail

namespace elle::core {

auto describe(CycleError error) noexcept -> std::string_view
{
    switch (error) {
    case CycleError::non_finite_parameter:
        return "a cycle parameter is not a finite number";
    case CycleError::too_many_passes:
        return "the cut schedule never reaches full depth";
    case CycleError::zero_step_down:
        return "step down must not be zero";
    default:
        return "unknown cycle error";
    }
}

} // namespace elle::core

#pragma once

#include <string>

namespace elle::core {

/// Rounds to six decimal places the way the original frontend prepared values for LinuxCNC
/// (`parseFloat(value.toFixed(6))`), including the JSON hop to the backend, which turns -0 into 0.
[[nodiscard]] auto round_for_linuxcnc(double value) -> double;

/// JavaScript `Math.round`: nearest integer, halves rounded towards positive infinity.
[[nodiscard]] auto round_half_up(double value) -> double;

/// Exact IEEE-754 equality. Only for places where exact comparison is the documented intent, such
/// as reproducing the reference cycle generators.
[[nodiscard]] auto exactly_equal(double lhs, double rhs) noexcept -> bool;

/// A G-code coordinate: fixed notation with six decimals, e.g. `-0.613435`.
[[nodiscard]] auto format_coordinate(double value) -> std::string;

/// Degrees to radians, rounded like Python's `math.radians` (multiplication by pi/180).
[[nodiscard]] auto radians(double degrees) noexcept -> double;

} // namespace elle::core

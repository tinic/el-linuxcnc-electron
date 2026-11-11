#pragma once

#include <elle/core/cycle.hpp>

namespace elle::core::detail {

/// Upper bound on generated passes. Real cycles need a few hundred at most; reaching this means the
/// parameters can never converge and the reference implementation would have looped forever.
inline constexpr int max_passes = 10'000;

/// Radius mode, metric, absolute, G1 feed, spindle on (G33 requires a running spindle).
auto append_preamble(GcodeLines& lines) -> void;

/// Makes the current tool position the given work coordinate (G10 L20 P1) and selects G54.
auto append_work_offset(GcodeLines& lines, const WorkPosition& position) -> void;

/// Converts an already rounded count to int, saturating outside int's range.
[[nodiscard]] auto saturating_count(double rounded) noexcept -> int;

} // namespace elle::core::detail

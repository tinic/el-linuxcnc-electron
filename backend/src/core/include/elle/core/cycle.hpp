#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace elle::core {

/// Position of the tool in work coordinates: X as a radius in mm, Z in mm, A in spindle revolutions.
struct WorkPosition {
    double x{};
    double z{};
    double a{};
};

/// Who consumes a generated program. The machine variant re-establishes the work coordinate system
/// and dwells before each threading pass; the preview variant is what the backplot interprets.
enum class Destination : std::uint8_t {
    machine,
    preview,
};

using GcodeLines = std::vector<std::string>;

enum class CycleError : std::uint8_t {
    non_finite_parameter,
    too_many_passes,
    zero_step_down,
};

[[nodiscard]] auto describe(CycleError error) noexcept -> std::string_view;

/// An entered parameter that prevents starting a cycle. `field` names the request member.
struct ParameterIssue {
    std::string_view field{};
    std::string_view problem{};
};

using ParameterIssues = std::vector<ParameterIssue>;

} // namespace elle::core

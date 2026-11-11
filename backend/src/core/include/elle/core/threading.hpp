#pragma once

#include <elle/core/cycle.hpp>

#include <expected>
#include <optional>

namespace elle::core {

/// Threading parameters as entered by the operator, in mm (X values as radius). An empty value
/// selects the default; zero is a value like any other.
struct ThreadingRequest {
    std::optional<double> pitch{};
    std::optional<double> x_depth{};
    std::optional<double> z_depth{};
    std::optional<double> angle_deg{};
    std::optional<double> z_end{};
    std::optional<double> x_pullout{};
    std::optional<double> z_pullout{};
    std::optional<double> first_cut{};
    std::optional<double> cut_mult{};
    std::optional<double> min_cut{};
    std::optional<double> spring_cuts{};
};

/// A fully resolved threading cycle in work coordinates, ready for G-code generation.
struct ThreadingProgram {
    double x_pos{};
    double z_pos{};
    double a_pos{};
    double x_start{};
    double z_start{};
    double pitch{};
    double x_depth{};
    double z_depth{};
    double x_end{};
    double z_end{};
    double x_return{};
    double z_return{};
    double x_pullout{};
    double z_pullout{};
    double first_cut{};
    double cut_mult{};
    double min_cut{};
    int spring_cuts{};
};

/// Checks the entered parameters. A cycle may only start when this returns no issues.
[[nodiscard]] auto validate_threading(const ThreadingRequest& request) -> ParameterIssues;

/// Resolves a request against the current tool position: applies defaults, the 4-pitch lead-in and
/// taper, and rounds every value to LinuxCNC's six decimals.
[[nodiscard]] auto plan_threading(const ThreadingRequest& request, const WorkPosition& position)
    -> ThreadingProgram;

/// Generates the G33 threading cycle. Fails instead of looping forever when the cut schedule can
/// never reach full depth.
[[nodiscard]] auto threading_gcode(const ThreadingProgram& program, Destination destination)
    -> std::expected<GcodeLines, CycleError>;

} // namespace elle::core

#pragma once

#include <elle/core/cycle.hpp>

#include <expected>
#include <optional>

namespace elle::core {

/// Turning parameters as entered by the operator, in mm (target as radius). An empty value selects
/// the default; zero is a value like any other.
struct TurningRequest {
    std::optional<double> target{};
    std::optional<double> z_end{};
    std::optional<double> feed_rate{};
    std::optional<double> step_down{};
    std::optional<double> final_step_down{};
    std::optional<double> taper_angle_deg{};
    std::optional<double> spring_passes{};
};

/// A fully resolved turning cycle in work coordinates, ready for G-code generation.
struct TurningProgram {
    double x_pos{};
    double z_pos{};
    double a_pos{};
    double pitch{};
    double stock{};
    double target{};
    double z_lead{};
    double z_end{};
    double angle_deg{};
    double step_down{};
    double final_step_down{};
    int spring_passes{};
    double x_return{};
    double z_return{};
};

/// Checks the entered parameters against the current tool position (the stock radius). A cycle may
/// only start when this returns no issues.
[[nodiscard]] auto validate_turning(const TurningRequest& request, const WorkPosition& position)
    -> ParameterIssues;

/// Resolves a request against the current tool position, which is taken as the stock radius.
[[nodiscard]] auto plan_turning(const TurningRequest& request, const WorkPosition& position)
    -> TurningProgram;

/// Generates the spindle-synchronized (G33) roughing, final and spring passes.
[[nodiscard]] auto turning_gcode(const TurningProgram& program, Destination destination)
    -> std::expected<GcodeLines, CycleError>;

} // namespace elle::core

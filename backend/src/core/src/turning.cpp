#include <elle/core/turning.hpp>

#include "gcode_common.hpp"

#include <elle/core/cycle.hpp>
#include <elle/core/numbers.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <expected>
#include <format>
#include <string>
#include <string_view>
#include <vector>

namespace elle::core {

namespace {

// Values the original frontend substituted for empty or zero entries.
constexpr double default_feed_rate = 0.2;
constexpr double default_step_down = 0.5;
constexpr double default_final_step_down = 0.1;

/// Distance in front of the work (in +Z) where every pass starts, to take up backlash.
constexpr double z_lead = 2.0;
/// Radial clearance above the stock when returning for the next pass.
constexpr double retract_clearance = 2.0;
/// Every pass starts and ends its return at Z0, the face of the work.
constexpr double z_start = 0.0;

auto all_finite(const TurningProgram& program) -> bool
{
    const std::array values{program.x_pos,     program.z_pos,  program.a_pos,
                            program.pitch,     program.stock,  program.target,
                            program.z_lead,    program.z_end,  program.angle_deg,
                            program.step_down, program.final_step_down,
                            program.x_return,  program.z_return};
    return std::ranges::all_of(values, [](double value) { return std::isfinite(value); });
}

struct Pass {
    std::string_view kind{};
    int number{};
    int of{};
    double depth{};
};

/// Roughing passes up to the finishing allowance, then the final pass and any spring passes at full
/// depth.
auto schedule_passes(const TurningProgram& program) -> std::expected<std::vector<Pass>, CycleError>
{
    const double total_depth = std::abs(program.stock - program.target);
    const double roughing_depth = total_depth - program.final_step_down;

    int roughing_passes = 0;
    if (roughing_depth > 0.0) {
        if (exactly_equal(program.step_down, 0.0)) {
            return std::unexpected{CycleError::zero_step_down};
        }
        const double needed = std::ceil(roughing_depth / program.step_down);
        if (needed > static_cast<double>(detail::max_passes)) {
            return std::unexpected{CycleError::too_many_passes};
        }
        roughing_passes = needed > 0.0 ? detail::saturating_count(needed) : 0;
    }
    if (program.spring_passes > detail::max_passes) {
        return std::unexpected{CycleError::too_many_passes};
    }

    std::vector<Pass> passes{};
    for (int index = 0; index < roughing_passes; ++index) {
        const double depth =
            std::min(static_cast<double>(index + 1) * program.step_down, roughing_depth);
        passes.push_back({.kind = "Roughing", .number = index + 1, .of = roughing_passes,
                          .depth = depth});
    }
    passes.push_back({.kind = "Final", .number = 1, .of = 1, .depth = total_depth});
    for (int index = 0; index < program.spring_passes; ++index) {
        passes.push_back({.kind = "Spring", .number = index + 1, .of = program.spring_passes,
                          .depth = total_depth});
    }
    return passes;
}

auto describe_pass(const Pass& pass) -> std::string
{
    if (pass.of > 1) {
        return std::format("({} pass {} of {})", pass.kind, pass.number, pass.of);
    }
    return std::format("({} pass)", pass.kind);
}

/// A 45° chamfer of `chamfer` on the finished diameter's edge at the face, in passes no deeper
/// than the step-down. Starts and ends with the tool at the retract diameter at Z0.
auto chamfer_passes(const TurningProgram& program, double chamfer)
    -> std::expected<GcodeLines, CycleError>
{
    if (!std::isfinite(chamfer)) {
        return std::unexpected{CycleError::non_finite_parameter};
    }
    const double step = program.step_down > 0.0 ? program.step_down : chamfer;
    const double needed = std::ceil(chamfer / step);
    if (needed > static_cast<double>(detail::max_passes)) {
        return std::unexpected{CycleError::too_many_passes};
    }
    const int passes = detail::saturating_count(needed);
    const double pitch = std::abs(program.pitch);
    const double taper = std::tan(radians(program.angle_deg));
    const double retract_x = program.stock + retract_clearance;

    GcodeLines lines{};
    for (int pass = 1; pass <= passes; ++pass) {
        const double size = chamfer * static_cast<double>(pass) / static_cast<double>(passes);
        // The 45° line through (target - size, Z0) and the finished surface at Z = -size, entered
        // from in front of the face and never below the centreline.
        const double lead = std::min(z_lead, program.target - size);
        lines.push_back(std::format("(chamfer pass {} of {})", pass, passes));
        lines.push_back(std::format("G0 Z{}", format_coordinate(lead)));
        lines.push_back(std::format("G0 X{}", format_coordinate(program.target - size - lead)));
        lines.push_back(std::format("G33 X{} Z{} K{}",
                                    format_coordinate(program.target + (size * taper)),
                                    format_coordinate(-size), format_coordinate(pitch)));
        lines.push_back(std::format("G0 X{}", format_coordinate(retract_x)));
    }
    lines.push_back(std::format("G0 Z{}", format_coordinate(z_start)));
    return lines;
}

} // namespace

auto plan_turning(const TurningRequest& request, const WorkPosition& position) -> TurningProgram
{
    const double target = request.target.value_or(0.0);
    return TurningProgram{
        .x_pos = round_for_linuxcnc(position.x),
        .z_pos = round_for_linuxcnc(position.z),
        .a_pos = round_for_linuxcnc(position.a),
        .pitch = round_for_linuxcnc(request.feed_rate.value_or(default_feed_rate)),
        .stock = round_for_linuxcnc(position.x),
        .target = round_for_linuxcnc(target),
        .z_lead = round_for_linuxcnc(z_lead),
        .z_end = round_for_linuxcnc(request.z_end.value_or(0.0)),
        .angle_deg = round_for_linuxcnc(request.taper_angle_deg.value_or(0.0)),
        .step_down = round_for_linuxcnc(request.step_down.value_or(default_step_down)),
        .final_step_down = round_for_linuxcnc(
            request.final_step_down.value_or(default_final_step_down)),
        .spring_passes = detail::saturating_count(
            round_half_up(request.spring_passes.value_or(0.0))),
        .x_return = round_for_linuxcnc(target),
        .z_return = round_for_linuxcnc(position.z),
    };
}

auto turning_gcode(const TurningProgram& program, Destination destination)
    -> std::expected<GcodeLines, CycleError>
{
    if (!all_finite(program)) {
        return std::unexpected{CycleError::non_finite_parameter};
    }
    const auto passes = schedule_passes(program);
    if (!passes.has_value()) {
        return std::unexpected{passes.error()};
    }

    GcodeLines lines{};
    detail::append_preamble(lines);
    if (destination == Destination::machine) {
        detail::append_work_offset(lines, {.x = program.x_pos, .z = program.z_pos, .a = 0.0});
    }
    lines.push_back(std::format("G0 X{} Z{}", format_coordinate(program.stock),
                                format_coordinate(z_start)));

    const double pitch = std::abs(program.pitch);
    const double taper = std::tan(radians(program.angle_deg));
    const double z_travel = program.z_end - z_start;
    const double retract_x = program.stock + retract_clearance;

    for (const Pass& pass : *passes) {
        const double cut_radius = program.stock - pass.depth;
        lines.push_back(describe_pass(pass));
        lines.push_back(std::format("G0 X{} Z{}",
                                    format_coordinate(cut_radius - (program.z_lead * taper)),
                                    format_coordinate(program.z_lead)));
        lines.push_back(std::format("G33 X{} Z{} K{}",
                                    format_coordinate(cut_radius - (z_travel * taper)),
                                    format_coordinate(program.z_end), format_coordinate(pitch)));
        lines.push_back(std::format("G0 X{}", format_coordinate(retract_x)));
        lines.push_back(std::format("G0 Z{}", format_coordinate(z_start)));
    }

    lines.push_back(std::format("G0 X{} Z{}", format_coordinate(program.x_return),
                                format_coordinate(program.z_return)));
    return lines;
}

auto turning_gcode(const TurningProgram& program, Destination destination, double chamfer)
    -> std::expected<GcodeLines, CycleError>
{
    auto result = turning_gcode(program, destination);
    if (result.has_value() && chamfer > 0.0) {
        if (auto passes = chamfer_passes(program, chamfer); passes.has_value()) {
            // Before the final return to the start position.
            result->insert(result->end() - 1, passes->begin(), passes->end());
        } else {
            result = std::unexpected{passes.error()};
        }
    }
    return result;
}

} // namespace elle::core

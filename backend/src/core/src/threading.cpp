#include <elle/core/threading.hpp>

#include "gcode_common.hpp"

#include <elle/core/cycle.hpp>
#include <elle/core/numbers.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <expected>
#include <format>
#include <numbers>

namespace elle::core {

namespace {

// Values the original frontend substituted for empty or zero entries.
constexpr double default_x_pullout = 0.1;
constexpr double default_z_pullout = 0.1;
constexpr double default_first_cut = 0.1;
constexpr double default_cut_mult = 0.8;
constexpr double default_min_cut = 0.05;
constexpr double default_spring_cuts = 1.0;

/// The cycle starts this many pitches in front of the work so the carriage is synchronized.
constexpr double lead_in_pitches = 4.0;
constexpr double degrees_per_half_turn = 180.0;

/// The taper as the original frontend computed it: (angle * pi) / 180, not angle * (pi / 180).
auto taper_tangent(double angle_deg) -> double
{
    return std::tan((angle_deg * std::numbers::pi) / degrees_per_half_turn);
}

auto all_finite(const ThreadingProgram& program) -> bool
{
    const std::array values{program.x_pos,     program.z_pos,     program.a_pos,
                            program.x_start,   program.z_start,   program.pitch,
                            program.x_depth,   program.z_depth,   program.x_end,
                            program.z_end,     program.x_return,  program.z_return,
                            program.x_pullout, program.z_pullout, program.first_cut,
                            program.cut_mult,  program.min_cut};
    return std::ranges::all_of(values, [](double value) { return std::isfinite(value); });
}

/// Where the tool is cutting relative to the thread's start, and how deep the last cut was.
struct CutState {
    double x_cut{};
    double z_cut{};
    double cut_size{};
};

/// Unit vector of the infeed direction. Straight threads feed along X only; compound infeed also
/// moves Z so the tool cuts on one flank.
struct Infeed {
    double k_x{};
    double k_z{};
};

/// Constant per-cycle values after taking magnitudes, as the reference generator did.
struct Schedule {
    double pitch{};
    double first_cut{};
    double cut_mult{};
    double min_cut{};
};

auto advance_cut(CutState& state, const Schedule& schedule, const Infeed& infeed,
                 const ThreadingProgram& program) -> void
{
    // The cut size is never negative, so "not positive" is the reference's "== 0.0".
    state.cut_size = (state.cut_size <= 0.0) ? schedule.first_cut
                                             : state.cut_size * schedule.cut_mult;
    if (std::abs(state.cut_size) < std::abs(schedule.min_cut)) {
        state.cut_size = schedule.min_cut;
    }
    state.x_cut = state.x_cut + (state.cut_size * infeed.k_x);
    state.z_cut = state.z_cut + (state.cut_size * infeed.k_z);
    if (std::abs(state.x_cut) >= std::abs(program.x_depth)) {
        state.x_cut = program.x_depth;
        state.z_cut = program.z_depth;
    }
}

auto append_pass(GcodeLines& lines, int pass_number, const CutState& state,
                 const ThreadingProgram& program, const Schedule& schedule,
                 Destination destination) -> void
{
    const double cut_end_z = program.z_end + state.z_cut;
    const double pullout_z = cut_end_z + program.z_pullout;
    const auto x_end = format_coordinate(program.x_end);
    const auto pitch = format_coordinate(schedule.pitch);

    lines.push_back(std::format("(Pass {} - Cut size: {:.4f})", pass_number, state.cut_size));
    lines.push_back(std::format("G1 X{} Z{}", format_coordinate(program.x_start + state.x_cut),
                                format_coordinate(program.z_start + state.z_cut)));
    if (destination == Destination::machine) {
        lines.emplace_back("G4 P0.01");
    }
    lines.push_back(std::format("G33 X{} Z{} K{}", format_coordinate(program.x_end + state.x_cut),
                                format_coordinate(cut_end_z), pitch));
    lines.push_back(std::format("G33 X{} Z{} K{}", x_end, format_coordinate(pullout_z), pitch));
    lines.push_back(std::format("G1 X{} Z{}", x_end, format_coordinate(pullout_z)));
    lines.push_back(std::format("G0 X{}", format_coordinate(program.x_end + program.x_pullout)));
    lines.push_back(std::format("G0 Z{}", format_coordinate(program.z_start)));
    lines.push_back(std::format("G0 X{}", format_coordinate(program.x_start)));
}

} // namespace

auto plan_threading(const ThreadingRequest& request, const WorkPosition& position)
    -> ThreadingProgram
{
    const double pitch = request.pitch.value_or(0.0);
    const double z_depth = request.z_depth.value_or(0.0);
    const double angle = request.angle_deg.value_or(0.0);
    const double z_end = request.z_end.value_or(0.0);

    const double lead_in = pitch * lead_in_pitches;
    constexpr double user_z_start = 0.0;
    const double thread_length = std::abs((z_end - user_z_start) + z_depth);
    const bool tapered = !exactly_equal(angle, 0.0);
    const double x_end_offset = tapered ? thread_length * taper_tangent(angle) : 0.0;
    const double z_start_offset = user_z_start + lead_in;
    const double x_start_offset = tapered ? lead_in * taper_tangent(angle) : 0.0;

    return ThreadingProgram{
        .x_pos = round_for_linuxcnc(position.x),
        .z_pos = round_for_linuxcnc(position.z),
        .a_pos = round_for_linuxcnc(position.a),
        .x_start = round_for_linuxcnc(position.x - x_start_offset),
        .z_start = round_for_linuxcnc(position.z + z_start_offset),
        .pitch = round_for_linuxcnc(pitch),
        .x_depth = round_for_linuxcnc(request.x_depth.value_or(0.0)),
        .z_depth = round_for_linuxcnc(z_depth),
        .x_end = round_for_linuxcnc(position.x + x_end_offset),
        .z_end = round_for_linuxcnc(position.z + z_end),
        .x_return = round_for_linuxcnc(position.x),
        .z_return = round_for_linuxcnc(position.z),
        .x_pullout = round_for_linuxcnc(request.x_pullout.value_or(default_x_pullout)),
        .z_pullout = round_for_linuxcnc(request.z_pullout.value_or(default_z_pullout)),
        .first_cut = round_for_linuxcnc(request.first_cut.value_or(default_first_cut)),
        .cut_mult = round_for_linuxcnc(request.cut_mult.value_or(default_cut_mult)),
        .min_cut = round_for_linuxcnc(request.min_cut.value_or(default_min_cut)),
        .spring_cuts = detail::saturating_count(
            round_half_up(request.spring_cuts.value_or(default_spring_cuts))),
    };
}

auto threading_gcode(const ThreadingProgram& program, Destination destination)
    -> std::expected<GcodeLines, CycleError>
{
    if (!all_finite(program)) {
        return std::unexpected{CycleError::non_finite_parameter};
    }
    const Schedule schedule{
        .pitch = std::abs(program.pitch),
        .first_cut = std::abs(program.first_cut),
        .cut_mult = std::abs(program.cut_mult),
        .min_cut = std::abs(program.min_cut),
    };
    const double compound_depth =
        std::sqrt((program.x_depth * program.x_depth) + (program.z_depth * program.z_depth));
    const Infeed infeed = (compound_depth > 0.0) ? Infeed{.k_x = program.x_depth / compound_depth,
                                                          .k_z = program.z_depth / compound_depth}
                                                 : Infeed{};

    GcodeLines lines{};
    detail::append_preamble(lines);
    if (destination == Destination::machine) {
        detail::append_work_offset(lines, {.x = program.x_pos, .z = program.z_pos, .a = 0.0});
    }
    lines.push_back(std::format("G0 X{} Z{}", format_coordinate(program.x_start),
                                format_coordinate(program.z_start)));

    CutState state{};
    int spring_cuts_remaining = program.spring_cuts;
    for (int pass_number = 1; spring_cuts_remaining >= 0; ++pass_number) {
        if (pass_number > detail::max_passes) {
            return std::unexpected{CycleError::too_many_passes};
        }
        advance_cut(state, schedule, infeed, program);
        append_pass(lines, pass_number, state, program, schedule, destination);
        if (exactly_equal(std::abs(state.x_cut), std::abs(program.x_depth))) {
            if (spring_cuts_remaining > 0) {
                // Back off one cut so the spring pass re-cuts the last increment at full depth.
                state.x_cut = state.x_cut - (state.cut_size * infeed.k_x);
                state.z_cut = state.z_cut - (state.cut_size * infeed.k_z);
            }
            --spring_cuts_remaining;
        }
    }

    lines.push_back(std::format("G0 X{} Z{}", format_coordinate(program.x_return),
                                format_coordinate(program.z_return)));
    return lines;
}

} // namespace elle::core

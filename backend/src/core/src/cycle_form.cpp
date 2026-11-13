#include <elle/core/cycle_form.hpp>

#include <elle/core/cycle.hpp>
#include <elle/core/numbers.hpp>
#include <elle/core/threading.hpp>
#include <elle/core/turning.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <span>
#include <string_view>
#include <utility>

namespace elle::core {

namespace {

/// Flank infeed beyond this makes no sense for any common thread form.
constexpr double max_infeed_deg = 45.0;
/// Radial clearance for the return when none is entered, as the shipped presets use.
constexpr double default_retract = 0.5;

struct KeyMap {
    std::string_view request{};
    std::string_view form{};
};

constexpr auto threading_keys = std::to_array<KeyMap>({
    {.request = "pitch", .form = "pitch"},
    {.request = "x_depth", .form = "depth"},
    {.request = "z_depth", .form = "infeed_deg"},
    {.request = "angle_deg", .form = "taper_deg"},
    {.request = "z_end", .form = "length"},
    {.request = "x_pullout", .form = "retract"},
    {.request = "z_pullout", .form = "run_out"},
    {.request = "first_cut", .form = "first_cut"},
    {.request = "cut_mult", .form = "cut_factor"},
    {.request = "min_cut", .form = "min_cut"},
    {.request = "spring_cuts", .form = "spring_cuts"},
});

constexpr auto turning_keys = std::to_array<KeyMap>({{.request = "z_end", .form = "length"}});

/// Adds the planner's issues for the converted request under the form's field names, except for
/// fields the form checks already flagged.
auto add_planner_issues(ParameterIssues& issues, const ParameterIssues& planner,
                        std::span<const KeyMap> keys) -> void
{
    for (const ParameterIssue& issue : planner) {
        std::string_view field = issue.field;
        for (const KeyMap& key : keys) {
            if (key.request == issue.field) {
                field = key.form;
            }
        }
        if (std::ranges::find(issues, field, &ParameterIssue::field) == issues.end()) {
            issues.push_back({.field = field, .problem = issue.problem});
        }
    }
}

/// The threading inputs after defaults, with the form's own checks.
struct ThreadingInputs {
    bool internal{};
    std::optional<double> depth{};
    std::optional<double> length{};
    double infeed{};
    double run_out{};
    double retract{};
    ParameterIssues issues{}; ///< the form's own checks

    /// Depth and length are usable, so the thread's geometry can be worked out.
    [[nodiscard]] auto convertible() const -> bool
    {
        return depth.value_or(0.0) > 0.0 && length.value_or(0.0) > 0.0;
    }
};

auto threading_inputs(const ThreadingForm& form) -> ThreadingInputs
{
    const double pitch = form.pitch.value_or(0.0);
    ThreadingInputs inputs{.internal = form.internal,
                           .depth = form.depth,
                           .length = form.length,
                           .infeed = form.infeed_deg.value_or(0.0),
                           .run_out = form.run_out.value_or(pitch),
                           .retract = form.retract.value_or(default_retract)};
    if (!inputs.depth.has_value() && pitch > 0.0) {
        inputs.depth =
            pitch * (form.internal ? internal_depth_per_pitch : external_depth_per_pitch);
    }
    if (inputs.depth.has_value() && *inputs.depth <= 0.0) {
        inputs.issues.push_back({.field = "depth", .problem = "must be greater than zero"});
    }
    if (inputs.infeed < 0.0 || inputs.infeed > max_infeed_deg) {
        inputs.issues.push_back({.field = "infeed_deg", .problem = "must be between 0 and 45 degrees"});
    }
    if (inputs.run_out < 0.0) {
        inputs.issues.push_back({.field = "run_out", .problem = "must not be negative"});
    }
    if (inputs.retract <= 0.0) {
        inputs.issues.push_back({.field = "retract", .problem = "must be greater than zero"});
    }
    if (!form.length.has_value()) {
        inputs.issues.push_back({.field = "length", .problem = "required"});
    } else if (*form.length <= 0.0) {
        inputs.issues.push_back({.field = "length", .problem = "must be greater than zero"});
    }
    return inputs;
}

/// Signs, infeed and the thread end in the planner's terms.
auto threading_geometry(const ThreadingInputs& inputs, const WorkPosition& position,
                        FormConversion<ThreadingRequest>& result) -> void
{
    ThreadingRequest& request = result.request;
    const double depth = inputs.depth.value_or(0.0);
    const double length = inputs.length.value_or(0.0);
    // External threads cut inward (-X) and retract outward, internal ones the other way.
    const double inward = inputs.internal ? 1.0 : -1.0;
    request.x_depth = inward * depth;
    request.x_pullout = -inward * inputs.retract;

    // Flank infeed: each pass starts further toward the chuck, along the flank behind the tool,
    // so that only its leading edge cuts. Full depth is shifted by `slant` in -Z.
    const double slant = depth * std::tan(radians(inputs.infeed));
    request.z_depth = -slant;

    // The thread ends at Z = -length (work coordinates). The angled retraction (run-out)
    // continues toward the chuck and finishes exactly there on the full-depth pass, so no pass
    // goes beyond the end. The planner takes the end relative to the tool's Z.
    const double cutting_end = -length + inputs.run_out + slant;
    request.z_pullout = -inputs.run_out;
    request.z_end = cutting_end - position.z;
    if (cutting_end >= position.z) {
        result.issues.push_back(
            {.field = "length",
             .problem = "must end beyond the tool: place the tool in front of the thread"});
    }
}

} // namespace

auto threading_request(const ThreadingForm& form, const WorkPosition& position)
    -> FormConversion<ThreadingRequest>
{
    FormConversion<ThreadingRequest> result{};
    ThreadingRequest& request = result.request;
    request.pitch = form.pitch;
    request.angle_deg = form.taper_deg.value_or(0.0);
    request.first_cut = form.first_cut;
    request.cut_mult = form.cut_factor;
    request.min_cut = form.min_cut;
    request.spring_cuts = form.spring_cuts;

    ThreadingInputs inputs = threading_inputs(form);
    result.issues = std::move(inputs.issues);
    // Convert whenever depth and length are usable, so that the planner's checks below report the
    // remaining fields correctly even while another field is still invalid.
    if (inputs.convertible()) {
        threading_geometry(inputs, position, result);
    }
    add_planner_issues(result.issues, validate_threading(request), threading_keys);
    return result;
}

auto turning_request(const TurningForm& form, const WorkPosition& position)
    -> FormConversion<TurningRequest>
{
    FormConversion<TurningRequest> result{};
    TurningRequest& request = result.request;
    request.target = form.target;
    request.feed_rate = form.feed_rate;
    request.step_down = form.step_down;
    request.final_step_down = form.final_step_down;
    request.taper_angle_deg = form.taper_angle_deg;
    request.spring_passes = form.spring_passes;
    if (form.length.has_value()) {
        if (*form.length <= 0.0) {
            result.issues.push_back({.field = "length", .problem = "must be greater than zero"});
        } else {
            request.z_end = -*form.length;
        }
    }
    add_planner_issues(result.issues, validate_turning(request, position), turning_keys);
    return result;
}

} // namespace elle::core

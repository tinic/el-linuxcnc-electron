// Parameter checks before a cycle may start. The original frontend had these rules but never
// enforced the threading ones (it tested the returned list as a boolean), and had no check that
// turning actually removes material.

#include <elle/core/cycle.hpp>
#include <elle/core/threading.hpp>
#include <elle/core/turning.hpp>

#include <cmath>
#include <optional>
#include <string_view>
#include <utility>

namespace elle::core {

namespace {

constexpr double max_pitch = 10.0;
constexpr double max_thread_depth = 10.0;
constexpr double min_cut_mult = 0.5;
constexpr double max_cut_mult = 1.0;
constexpr double max_extra_passes = 10.0;
constexpr double max_taper_deg = 45.0;

struct Range {
    double low{};
    double high{};
};

class Checker {
public:
    /// The value if it was entered and is a finite number; records an issue if it is not finite.
    auto entered(std::string_view field, const std::optional<double>& value)
        -> std::optional<double>
    {
        if (!value.has_value()) {
            return std::nullopt;
        }
        if (!std::isfinite(*value)) {
            add(field, "must be a number");
            return std::nullopt;
        }
        return value;
    }

    auto required(std::string_view field, const std::optional<double>& value)
        -> std::optional<double>
    {
        if (!value.has_value()) {
            add(field, "is required");
            return std::nullopt;
        }
        return entered(field, value);
    }

    auto positive(std::string_view field, const std::optional<double>& value) -> void
    {
        if (const auto checked = entered(field, value); checked.has_value() && *checked <= 0.0) {
            add(field, "must be greater than zero");
        }
    }

    auto within(std::string_view field, const std::optional<double>& value, Range range,
                std::string_view problem) -> void
    {
        if (const auto checked = entered(field, value);
            checked.has_value() && (*checked < range.low || *checked > range.high)) {
            add(field, problem);
        }
    }

    auto add(std::string_view field, std::string_view problem) -> void
    {
        issues_.push_back({.field = field, .problem = problem});
    }

    [[nodiscard]] auto issues() && -> ParameterIssues { return std::move(issues_); }

private:
    ParameterIssues issues_{};
};

} // namespace

auto validate_threading(const ThreadingRequest& request) -> ParameterIssues
{
    Checker check{};
    if (const auto pitch = check.required("pitch", request.pitch); pitch.has_value()) {
        if (*pitch <= 0.0) {
            check.add("pitch", "must be greater than zero");
        } else if (*pitch > max_pitch) {
            check.add("pitch", "must not exceed 10 mm");
        }
    }
    static_cast<void>(check.required("z_end", request.z_end));
    check.within("x_depth", request.x_depth, {.low = -max_thread_depth, .high = max_thread_depth},
                 "must not exceed 10 mm");
    static_cast<void>(check.entered("z_depth", request.z_depth));
    static_cast<void>(check.entered("angle_deg", request.angle_deg));
    static_cast<void>(check.entered("x_pullout", request.x_pullout));
    static_cast<void>(check.entered("z_pullout", request.z_pullout));
    check.positive("first_cut", request.first_cut);
    check.within("cut_mult", request.cut_mult, {.low = min_cut_mult, .high = max_cut_mult},
                 "must be between 0.5 and 1.0");
    check.positive("min_cut", request.min_cut);
    check.within("spring_cuts", request.spring_cuts, {.low = 0.0, .high = max_extra_passes},
                 "must be between 0 and 10");
    return std::move(check).issues();
}

auto validate_turning(const TurningRequest& request, const WorkPosition& position)
    -> ParameterIssues
{
    Checker check{};
    if (const auto target = check.required("target", request.target);
        target.has_value() && *target >= position.x) {
        check.add("target", "must be smaller than the stock at the tool position");
    }
    if (const auto z_end = check.required("z_end", request.z_end);
        z_end.has_value() && *z_end >= 0.0) {
        check.add("z_end", "must be negative (into the work)");
    }
    check.positive("feed_rate", request.feed_rate);
    check.positive("step_down", request.step_down);
    check.positive("final_step_down", request.final_step_down);
    check.within("taper_angle_deg", request.taper_angle_deg,
                 {.low = -max_taper_deg, .high = max_taper_deg},
                 "must be between -45 and 45 degrees");
    check.within("spring_passes", request.spring_passes, {.low = 0.0, .high = max_extra_passes},
                 "must be between 0 and 10");
    return std::move(check).issues();
}

} // namespace elle::core

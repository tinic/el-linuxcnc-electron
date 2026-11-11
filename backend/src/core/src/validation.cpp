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

class Checker {
public:
    auto required(std::string_view field, const std::optional<double>& value) -> bool
    {
        if (!value.has_value()) {
            add(field, "is required");
            return false;
        }
        return finite(field, value);
    }

    auto finite(std::string_view field, const std::optional<double>& value) -> bool
    {
        if (value.has_value() && !std::isfinite(*value)) {
            add(field, "must be a number");
            return false;
        }
        return value.has_value();
    }

    auto positive(std::string_view field, const std::optional<double>& value) -> void
    {
        if (finite(field, value) && *value <= 0.0) {
            add(field, "must be greater than zero");
        }
    }

    auto within(std::string_view field, const std::optional<double>& value, double low, double high,
                std::string_view problem) -> void
    {
        if (finite(field, value) && (*value < low || *value > high)) {
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
    if (check.required("pitch", request.pitch)) {
        if (*request.pitch <= 0.0) {
            check.add("pitch", "must be greater than zero");
        } else if (*request.pitch > max_pitch) {
            check.add("pitch", "must not exceed 10 mm");
        }
    }
    check.required("z_end", request.z_end);
    check.within("x_depth", request.x_depth, -max_thread_depth, max_thread_depth,
                 "must not exceed 10 mm");
    check.finite("z_depth", request.z_depth);
    check.finite("angle_deg", request.angle_deg);
    check.finite("x_pullout", request.x_pullout);
    check.finite("z_pullout", request.z_pullout);
    check.positive("first_cut", request.first_cut);
    check.within("cut_mult", request.cut_mult, min_cut_mult, max_cut_mult,
                 "must be between 0.5 and 1.0");
    check.positive("min_cut", request.min_cut);
    check.within("spring_cuts", request.spring_cuts, 0.0, max_extra_passes,
                 "must be between 0 and 10");
    return std::move(check).issues();
}

auto validate_turning(const TurningRequest& request, const WorkPosition& position)
    -> ParameterIssues
{
    Checker check{};
    if (check.required("target", request.target) && *request.target >= position.x) {
        check.add("target", "must be smaller than the stock at the tool position");
    }
    if (check.required("z_end", request.z_end) && *request.z_end >= 0.0) {
        check.add("z_end", "must be negative (into the work)");
    }
    check.positive("feed_rate", request.feed_rate);
    check.positive("step_down", request.step_down);
    check.positive("final_step_down", request.final_step_down);
    check.within("taper_angle_deg", request.taper_angle_deg, -max_taper_deg, max_taper_deg,
                 "must be between -45 and 45 degrees");
    check.within("spring_passes", request.spring_passes, 0.0, max_extra_passes,
                 "must be between 0 and 10");
    return std::move(check).issues();
}

} // namespace elle::core

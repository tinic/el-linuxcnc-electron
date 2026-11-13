#pragma once

#include <elle/core/cycle.hpp>
#include <elle/core/threading.hpp>
#include <elle/core/turning.hpp>

#include <optional>

namespace elle::core {

/// What the operator enters for a threading cycle, in the conventions of conversational lathe
/// controls: positive numbers, the side chosen once, an infeed angle, and the length measured from
/// the work face (Z0). `threading_request` turns it into the planner's ThreadingRequest.
struct ThreadingForm {
    std::optional<double> pitch{};
    bool internal{};                     ///< internal thread (bore) instead of external
    std::optional<double> depth{};       ///< radial thread depth; empty: from the pitch
    std::optional<double> infeed_deg{};  ///< 0 straight in; 29.5 flank infeed (60° threads)
    std::optional<double> length{};      ///< thread ends this far from the face (Z0), positive
    std::optional<double> taper_deg{};   ///< 0 for straight threads; NPT 1.7899
    std::optional<double> run_out{};     ///< angled retraction, finishing at the thread end
    std::optional<double> retract{};     ///< radial clearance for the return
    std::optional<double> first_cut{};
    std::optional<double> cut_factor{};  ///< each cut is the previous one times this
    std::optional<double> min_cut{};
    std::optional<double> spring_cuts{};
};

/// What the operator enters for a turning cycle. Only the length differs from TurningRequest: it is
/// positive, measured from the face, where TurningRequest takes the end as a negative Z.
struct TurningForm {
    std::optional<double> target{};
    std::optional<double> length{};
    std::optional<double> feed_rate{};
    std::optional<double> step_down{};
    std::optional<double> final_step_down{};
    std::optional<double> taper_angle_deg{};
    std::optional<double> spring_passes{};
};

/// Radial depth of a 60° thread (ISO metric and Unified) per unit of pitch.
inline constexpr double external_depth_per_pitch = 0.61343;
inline constexpr double internal_depth_per_pitch = 0.54127;

template <typename Request>
struct FormConversion {
    Request request{};
    ParameterIssues issues{}; ///< keyed by form field
};

/// The planner's request for a threading form, with the tool at `position`. Issues (form keys)
/// include the planner's own validation of the converted request.
[[nodiscard]] auto threading_request(const ThreadingForm& form, const WorkPosition& position)
    -> FormConversion<ThreadingRequest>;

[[nodiscard]] auto turning_request(const TurningForm& form, const WorkPosition& position)
    -> FormConversion<TurningRequest>;

} // namespace elle::core

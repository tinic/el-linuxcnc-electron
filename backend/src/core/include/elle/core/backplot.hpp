#pragma once

#include <elle/core/cycle.hpp>

#include <cstddef>
#include <cstdint>
#include <expected>
#include <string>
#include <vector>

namespace elle::core {

enum class MoveKind : std::uint8_t {
    rapid,  ///< G0
    feed,   ///< G1
    thread, ///< G33, spindle-synchronized
};

struct PlotPoint {
    double x{}; ///< radius, mm
    double z{}; ///< mm
};

/// One straight move of the tool in the XZ plane.
struct Segment {
    MoveKind kind{MoveKind::rapid};
    PlotPoint from{};
    PlotPoint to{};
    std::size_t line{}; ///< index of the G-code line that produced it
};

struct Bounds {
    PlotPoint min{};
    PlotPoint max{};
};

struct Backplot {
    std::vector<Segment> segments{};
    Bounds bounds{};
};

struct BackplotError {
    std::size_t line{};
    std::string message{};
};

/// Interprets the subset of G-code Elle's cycles emit (G0/G1/G33 with absolute X/Z words; setup
/// codes, dwells and comments are skipped) into the tool path, starting at `start`. Every word
/// that is not understood is an error rather than silently ignored, so the preview can never show
/// a different path from the one that would run.
[[nodiscard]] auto backplot(const GcodeLines& lines, PlotPoint start)
    -> std::expected<Backplot, BackplotError>;

} // namespace elle::core

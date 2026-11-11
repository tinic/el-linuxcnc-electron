#include <elle/core/backplot.hpp>

#include <elle/core/cycle.hpp>

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <expected>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace elle::core {

namespace {

/// Setup codes the cycles emit that do not move the tool in the preview.
constexpr auto non_motion_codes =
    std::to_array<std::string_view>({"G8", "G21", "G90", "G54", "F100", "M3S500"});

struct Words {
    std::optional<MoveKind> motion{};
    std::optional<double> x{};
    std::optional<double> z{};
};

auto parse_number(std::string_view text) -> std::optional<double>
{
    double value{};
    const auto result = std::from_chars(text.data(), std::to_address(text.end()), value);
    if (result.ec != std::errc{} || result.ptr != std::to_address(text.end())) {
        return std::nullopt;
    }
    return value;
}

auto motion_for(std::string_view word) -> std::optional<MoveKind>
{
    if (word == "G0") {
        return MoveKind::rapid;
    }
    if (word == "G1") {
        return MoveKind::feed;
    }
    if (word == "G33") {
        return MoveKind::thread;
    }
    return std::nullopt;
}

/// Splits one motion line into words. Returns an error message for anything not understood.
auto parse_motion(std::string_view line) -> std::expected<Words, std::string>
{
    Words words{};
    std::size_t position = 0;
    while (position < line.size()) {
        const std::size_t end = std::min(line.find(' ', position), line.size());
        const std::string_view word = line.substr(position, end - position);
        position = end + 1;
        if (word.empty()) {
            continue;
        }
        if (const auto motion = motion_for(word); motion.has_value() && !words.motion) {
            words.motion = motion;
            continue;
        }
        const auto value = parse_number(word.substr(1));
        if (!value.has_value()) {
            return std::unexpected{std::format("cannot read '{}'", word)};
        }
        switch (word.front()) {
        case 'X':
            words.x = value;
            break;
        case 'Z':
            words.z = value;
            break;
        case 'K': // thread pitch; the path itself does not depend on it
            break;
        default:
            return std::unexpected{std::format("unexpected word '{}'", word)};
        }
    }
    if (!words.motion.has_value()) {
        return std::unexpected{std::format("no motion in '{}'", line)};
    }
    return words;
}

auto skipped(std::string_view line) -> bool
{
    return line.empty() || line.starts_with('(') || line.starts_with("G4 ") ||
           line.starts_with("G10 ") || std::ranges::contains(non_motion_codes, line);
}

auto include(Bounds& bounds, PlotPoint point) -> void
{
    bounds.min = {.x = std::min(bounds.min.x, point.x), .z = std::min(bounds.min.z, point.z)};
    bounds.max = {.x = std::max(bounds.max.x, point.x), .z = std::max(bounds.max.z, point.z)};
}

} // namespace

auto backplot(const GcodeLines& lines, PlotPoint start) -> std::expected<Backplot, BackplotError>
{
    Backplot plot{.segments = {}, .bounds = {.min = start, .max = start}};
    PlotPoint tool = start;
    for (std::size_t index = 0; index < lines.size(); ++index) {
        const std::string_view line = lines.at(index);
        if (skipped(line)) {
            continue;
        }
        const auto words = parse_motion(line);
        if (!words.has_value()) {
            return std::unexpected{BackplotError{.line = index, .message = words.error()}};
        }
        const PlotPoint target{.x = words->x.value_or(tool.x), .z = words->z.value_or(tool.z)};
        plot.segments.push_back(
            {.kind = *words->motion, .from = tool, .to = target, .line = index});
        include(plot.bounds, target);
        tool = target;
    }
    return plot;
}

} // namespace elle::core

// The C++ cycle planners and generators must reproduce the original implementation exactly:
// golden/cycles.json was produced by running the original TypeScript planner (useCannedCycles.ts)
// and Python generator (lathe_halcomp.py) over every shipped preset. See golden/README.md.

#include <elle/core/cycle.hpp>
#include <elle/core/threading.hpp>
#include <elle/core/turning.hpp>

#include <glaze/glaze.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <format>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace {

using elle::core::Destination;
using elle::core::GcodeLines;

struct ThreadingCase {
    std::string name{};
    elle::core::WorkPosition position{};
    elle::core::ThreadingRequest request{};
    elle::core::ThreadingProgram program{};
    GcodeLines machine{};
    GcodeLines preview{};
};

struct TurningCase {
    std::string name{};
    elle::core::WorkPosition position{};
    elle::core::TurningRequest request{};
    elle::core::TurningProgram program{};
    GcodeLines machine{};
    GcodeLines preview{};
};

struct GoldenCycles {
    std::vector<ThreadingCase> threading{};
    std::vector<TurningCase> turning{};
};

struct StrictJson : glz::opts {
    bool error_on_missing_keys = true;
};

auto load_golden_cycles() -> GoldenCycles
{
    std::ifstream file{ELLE_GOLDEN_CYCLES_PATH};
    REQUIRE(file.is_open());
    const std::string buffer{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};

    GoldenCycles cycles{};
    const auto error = glz::read<StrictJson{}>(cycles, buffer);
    INFO(glz::format_error(error, buffer));
    REQUIRE(!error);
    return cycles;
}

const GoldenCycles& golden_cycles()
{
    static const GoldenCycles cycles = load_golden_cycles();
    return cycles;
}

/// Serializes a program so two programs compare field by field with exact (shortest round-trip)
/// numbers.
auto to_json(const auto& value) -> std::string
{
    return glz::write_json(value).value_or("<serialization failed>");
}

/// Empty when equal, otherwise a description of the first difference.
auto first_difference(const GcodeLines& actual, const GcodeLines& expected) -> std::string
{
    for (std::size_t line = 0; line < actual.size() && line < expected.size(); ++line) {
        if (actual.at(line) != expected.at(line)) {
            return std::format("line {}: got '{}', expected '{}'", line + 1, actual.at(line),
                               expected.at(line));
        }
    }
    if (actual.size() != expected.size()) {
        return std::format("got {} lines, expected {}", actual.size(), expected.size());
    }
    return {};
}

auto check_generated(const auto& generated, const GcodeLines& expected) -> void
{
    REQUIRE(generated.has_value());
    const std::string difference = first_difference(*generated, expected);
    INFO(difference);
    CHECK(difference.empty());
}

} // namespace

TEST_CASE("golden cycle set covers every shipped preset", "[golden]")
{
    constexpr std::size_t threading_cases = 104;
    constexpr std::size_t turning_cases = 60;
    CHECK(golden_cycles().threading.size() == threading_cases);
    CHECK(golden_cycles().turning.size() == turning_cases);
}

TEST_CASE("threading plans match the original frontend", "[golden][threading]")
{
    for (const ThreadingCase& golden : golden_cycles().threading) {
        INFO(golden.name);
        CHECK(to_json(elle::core::plan_threading(golden.request, golden.position)) ==
              to_json(golden.program));
    }
}

TEST_CASE("threading G-code matches the original backend", "[golden][threading]")
{
    for (const ThreadingCase& golden : golden_cycles().threading) {
        INFO(golden.name);
        check_generated(elle::core::threading_gcode(golden.program, Destination::machine),
                        golden.machine);
        check_generated(elle::core::threading_gcode(golden.program, Destination::preview),
                        golden.preview);
    }
}

TEST_CASE("turning plans match the original frontend", "[golden][turning]")
{
    for (const TurningCase& golden : golden_cycles().turning) {
        INFO(golden.name);
        CHECK(to_json(elle::core::plan_turning(golden.request, golden.position)) ==
              to_json(golden.program));
    }
}

TEST_CASE("turning G-code matches the original backend", "[golden][turning]")
{
    for (const TurningCase& golden : golden_cycles().turning) {
        INFO(golden.name);
        check_generated(elle::core::turning_gcode(golden.program, Destination::machine),
                        golden.machine);
        check_generated(elle::core::turning_gcode(golden.program, Destination::preview),
                        golden.preview);
    }
}

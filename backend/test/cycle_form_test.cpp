#include <elle/core/cycle.hpp>
#include <elle/core/cycle_form.hpp>
#include <elle/core/threading.hpp>
#include <elle/core/turning.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

using Catch::Approx;
using elle::core::Destination;
using elle::core::external_depth_per_pitch;
using elle::core::internal_depth_per_pitch;
using elle::core::ThreadingForm;
using elle::core::TurningForm;
using elle::core::WorkPosition;

namespace {

/// The value after `axis` in a G-code line ("G33 X-0.1 Z-6 K1"), if present.
auto word(std::string_view line, char axis) -> std::optional<double>
{
    const auto found = line.find(axis, 1);
    if (found == std::string_view::npos) {
        return std::nullopt;
    }
    const std::string_view rest = line.substr(found + 1);
    double value{};
    const auto [end, error] = std::from_chars(rest.data(), rest.data() + rest.size(), value);
    static_cast<void>(end);
    return error == std::errc{} ? std::optional<double>{value} : std::nullopt;
}

struct Move {
    double x{};
    double z{};
};

/// The synchronized (G33) moves of a program, in order.
auto synchronized_moves(const std::vector<std::string>& lines) -> std::vector<Move>
{
    std::vector<Move> moves{};
    for (const std::string& line : lines) {
        if (line.starts_with("G33 ")) {
            moves.push_back({.x = word(line, 'X').value_or(0.0), .z = word(line, 'Z').value_or(0.0)});
        }
    }
    return moves;
}

auto m6_external() -> ThreadingForm
{
    return {.pitch = 1.0, .length = 6.0, .run_out = 1.0, .retract = 0.5, .first_cut = 0.1,
            .cut_factor = 0.8, .min_cut = 0.025, .spring_cuts = 1.0};
}

auto program_lines(const ThreadingForm& form, const WorkPosition& position) -> std::vector<std::string>
{
    const auto converted = elle::core::threading_request(form, position);
    REQUIRE(converted.issues.empty());
    const auto program = elle::core::plan_threading(converted.request, position);
    const auto lines = elle::core::threading_gcode(program, Destination::machine);
    REQUIRE(lines.has_value());
    return *lines;
}

} // namespace

TEST_CASE("an external thread cuts inward and retracts outward", "[form][threading]")
{
    const auto converted = elle::core::threading_request(m6_external(), {.x = 3.0, .z = 0.0});
    REQUIRE(converted.issues.empty());
    CHECK(converted.request.x_depth.value_or(0.0) == Approx(-external_depth_per_pitch));
    CHECK(converted.request.x_pullout.value_or(0.0) == Approx(0.5));
}

TEST_CASE("an internal thread cuts outward and retracts inward", "[form][threading]")
{
    ThreadingForm form = m6_external();
    form.internal = true;
    const auto converted = elle::core::threading_request(form, {.x = 2.5, .z = 0.0});
    REQUIRE(converted.issues.empty());
    CHECK(converted.request.x_depth.value_or(0.0) == Approx(internal_depth_per_pitch));
    CHECK(converted.request.x_pullout.value_or(0.0) == Approx(-0.5));
}

TEST_CASE("an empty depth comes from the pitch", "[form][threading]")
{
    ThreadingForm form = m6_external();
    form.pitch = 1.5;
    form.depth = std::nullopt;
    const auto converted = elle::core::threading_request(form, {.x = 5.0, .z = 0.0});
    REQUIRE(converted.issues.empty());
    CHECK(converted.request.x_depth.value_or(0.0) == Approx(-1.5 * external_depth_per_pitch));
}

TEST_CASE("an entered depth is used as given", "[form][threading]")
{
    ThreadingForm form = m6_external();
    form.depth = 0.75;
    const auto converted = elle::core::threading_request(form, {.x = 5.0, .z = 0.0});
    CHECK(converted.request.x_depth.value_or(0.0) == Approx(-0.75));
}

TEST_CASE("synchronized moves never reverse along Z", "[form][threading]")
{
    // The original pulled out toward +Z (reversing the spindle-synced motion over the last
    // millimetre of thread); the run-out must keep moving toward the chuck.
    for (const double infeed : {0.0, 29.5}) {
        ThreadingForm form = m6_external();
        form.infeed_deg = infeed;
        const auto moves = synchronized_moves(program_lines(form, {.x = 3.0, .z = 0.0}));
        REQUIRE(moves.size() % 2 == 0);
        for (std::size_t pass = 0; pass + 1 < moves.size(); pass += 2) {
            CHECK(moves.at(pass + 1).z < moves.at(pass).z); // run-out continues toward the chuck
        }
        // External: every synchronized move stays at or inside the surface the tool started on.
        CHECK(std::ranges::all_of(moves, [](const Move& move) { return move.x <= 3.0 + 1e-9; }));
    }
}

TEST_CASE("no pass goes beyond the thread end; the full-depth pass reaches it", "[form][threading]")
{
    for (const double infeed : {0.0, 29.5}) {
        for (const double tool_z : {0.0, 2.0}) {
            ThreadingForm form = m6_external();
            form.infeed_deg = infeed;
            const auto moves = synchronized_moves(program_lines(form, {.x = 3.0, .z = tool_z}));
            const auto deepest = std::ranges::min(moves, {}, &Move::z);
            CHECK(deepest.z == Approx(-6.0).margin(1e-6));
        }
    }
}

TEST_CASE("flank infeed shifts each pass toward the chuck", "[form][threading]")
{
    ThreadingForm form = m6_external();
    form.infeed_deg = 29.5;
    const auto converted = elle::core::threading_request(form, {.x = 3.0, .z = 0.0});
    REQUIRE(converted.issues.empty());
    CHECK(converted.request.z_depth.value_or(0.0) < 0.0);
    CHECK(converted.request.z_depth.value_or(0.0) ==
          Approx(-external_depth_per_pitch * 0.5657727).epsilon(1e-4)); // tan 29.5°
}

TEST_CASE("the thread must end beyond the tool", "[form][threading]")
{
    ThreadingForm form = m6_external();
    form.length = 2.0;
    const auto converted = elle::core::threading_request(form, {.x = 3.0, .z = -1.5});
    REQUIRE(!converted.issues.empty());
    CHECK(converted.issues.front().field == "length");
}

TEST_CASE("planner issues use the form's field names", "[form][threading]")
{
    ThreadingForm form = m6_external();
    form.cut_factor = 1.5;
    form.infeed_deg = 60.0;
    const auto converted = elle::core::threading_request(form, {.x = 3.0, .z = 0.0});
    const auto has = [&](std::string_view field) {
        return std::ranges::any_of(converted.issues,
                                   [&](const auto& issue) { return issue.field == field; });
    };
    CHECK(has("cut_factor"));
    CHECK(has("infeed_deg"));
    CHECK(!has("length")); // entered and valid, even while other fields are not
}

TEST_CASE("turning takes the length from the face as a positive number", "[form][turning]")
{
    const TurningForm form{.target = 4.0, .length = 25.0, .feed_rate = 0.15, .step_down = 0.5,
                           .final_step_down = 0.1};
    const auto converted = elle::core::turning_request(form, {.x = 6.0, .z = 1.0});
    REQUIRE(converted.issues.empty());
    CHECK(converted.request.z_end.value_or(0.0) == Approx(-25.0));

    TurningForm backwards = form;
    backwards.length = -3.0;
    const auto rejected = elle::core::turning_request(backwards, {.x = 6.0, .z = 1.0});
    REQUIRE(rejected.issues.size() == 1);
    CHECK(rejected.issues.front().field == "length");
}

TEST_CASE("a chamfer of zero changes nothing", "[form][turning][chamfer]")
{
    const auto program = elle::core::plan_turning({.target = 4.0, .z_end = -20.0}, {.x = 5.0, .z = 0.0});
    const auto plain = elle::core::turning_gcode(program, Destination::machine);
    const auto none = elle::core::turning_gcode(program, Destination::machine, 0.0);
    REQUIRE(plain.has_value());
    REQUIRE(none.has_value());
    CHECK(*plain == *none);
}

namespace {

/// Where each chamfer pass enters: the Z it approaches at and the X it moves in to.
auto chamfer_entries(const std::vector<std::string>& lines) -> std::vector<Move>
{
    std::vector<Move> entries{};
    for (std::size_t index = 0; index + 2 < lines.size(); ++index) {
        if (lines.at(index).starts_with("(chamfer")) {
            entries.push_back({.x = word(lines.at(index + 2), 'X').value_or(-1.0),
                               .z = word(lines.at(index + 1), 'Z').value_or(-1.0)});
        }
    }
    return entries;
}

} // namespace

TEST_CASE("a chamfer is cut at 45° onto the finished size at the face", "[form][turning][chamfer]")
{
    const auto program = elle::core::plan_turning(
        {.target = 4.0, .z_end = -20.0, .step_down = 0.5}, {.x = 5.0, .z = 0.0});
    const auto lines = elle::core::turning_gcode(program, Destination::machine, 1.0);
    REQUIRE(lines.has_value());

    // 1 mm in steps of at most 0.5 mm, each entered in front of the face, above the centreline.
    const auto entries = chamfer_entries(*lines);
    CHECK(entries.size() == 2);
    CHECK(std::ranges::all_of(entries, [](const Move& entry) { return entry.z > 0.0 && entry.x >= 0.0; }));

    // The last synchronized move ends on the finished diameter, 1 mm from the face.
    const auto moves = synchronized_moves(*lines);
    if (moves.empty()) {
        FAIL("no synchronized moves");
    } else {
        CHECK(moves.back().x == Approx(4.0));
        CHECK(moves.back().z == Approx(-1.0));
    }
    // The cycle's final return move stays last.
    const auto plain = elle::core::turning_gcode(program, Destination::machine);
    REQUIRE(plain.has_value());
    CHECK(lines->back() == plain->back());
}

TEST_CASE("a chamfer must fit the finished part", "[form][turning][chamfer]")
{
    const TurningForm form{.target = 2.0, .length = 10.0, .chamfer = 2.0};
    const auto converted = elle::core::turning_request(form, {.x = 3.0, .z = 0.0});
    REQUIRE(!converted.issues.empty());
    CHECK(converted.issues.front().field == "chamfer");

    const TurningForm negative{.target = 2.0, .length = 10.0, .chamfer = -0.5};
    CHECK(elle::core::turning_request(negative, {.x = 3.0, .z = 0.0}).issues.front().field == "chamfer");
}

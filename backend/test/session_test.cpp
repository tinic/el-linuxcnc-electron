#include <elle/app/session.hpp>
#include <elle/app/settings.hpp>
#include <elle/app/task_worker.hpp>
#include <elle/core/lathe_controller.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

using elle::app::Outcome;
using elle::app::Session;
using std::chrono::milliseconds;

namespace {

/// A session with tools, whose settings file lives in the temporary directory.
class Fixture {
public:
    Fixture()
        : file_{std::filesystem::temp_directory_path() / "elle-session-test" / "settings.json"}
        , session_{elle::app::default_settings(), file_}
    {
        std::filesystem::remove_all(file_.parent_path());
        // The controller needs a first sample before positions mean anything.
        tick(10.0, 0.0);
    }

    auto send(std::string_view json) -> Outcome { return session_.handle(json, now_); }

    auto tick(double x, double z) -> Session::Tick
    {
        now_ += milliseconds{10};
        return session_.tick({.position_z = z, .position_x = x}, now_);
    }

    /// Runs enough control ticks for any encoder reset to settle, keeping the slides put.
    auto settle(double x, double z) -> void
    {
        for (int step = 0; step < 5; ++step) {
            static_cast<void>(tick(x, z));
        }
    }

    [[nodiscard]] auto settings_file() const -> const std::filesystem::path& { return file_; }
    auto session() -> Session& { return session_; }

private:
    std::filesystem::path file_{};
    Session session_;
    Session::Clock::time_point now_{};
};

auto contains(const std::vector<std::string>& replies, std::string_view text) -> bool
{
    return std::ranges::any_of(replies,
                               [text](const std::string& reply) { return reply.contains(text); });
}

auto has_run_cycle(const Outcome& outcome) -> bool
{
    return std::ranges::any_of(outcome.jobs, [](const elle::app::TaskJob& job) {
        return std::holds_alternative<elle::app::RunCycle>(job);
    });
}

constexpr std::string_view thread_m6 =
    R"({"type":"plan_threading","form":{"pitch":1,"internal":false,"depth":null,)"
    R"("infeed_deg":null,"length":6,"taper_deg":null,"run_out":null,"retract":null,)"
    R"("first_cut":null,"cut_factor":null,"min_cut":null,"spring_cuts":null}})";

} // namespace

TEST_CASE("the panel gets the settings when it is ready", "[session]")
{
    Fixture fixture{};
    const auto outcome = fixture.send(R"({"type":"ready"})");
    CHECK(contains(outcome.replies, R"("type":"settings")"));
    CHECK(contains(outcome.replies, "Reference Tool"));
}

TEST_CASE("malformed messages are reported, not acted on", "[session]")
{
    Fixture fixture{};
    CHECK(contains(fixture.send("{").replies, R"("level":"error")"));
    CHECK(contains(fixture.send(R"({"type":"warp_drive"})").replies, "unknown message type"));
    CHECK(contains(fixture.send(R"({"type":"set_feed","feed":"diagonal"})").replies,
                   R"("level":"error")"));
    CHECK(contains(fixture.send(R"({"type":"set_axis","axis":"x","value":1,"extra":2})").replies,
                   R"("level":"error")"));
}

TEST_CASE("a valid thread is planned with a preview", "[session]")
{
    Fixture fixture{};
    const auto outcome = fixture.send(thread_m6);
    REQUIRE(outcome.replies.size() == 1);
    CHECK(contains(outcome.replies, R"("type":"plan")"));
    CHECK(contains(outcome.replies, R"("issues":[])"));
    CHECK(contains(outcome.replies, R"("segments":[)"));
    CHECK(contains(outcome.replies, "G33"));
}

TEST_CASE("an invalid thread is answered with its issues", "[session]")
{
    Fixture fixture{};
    const auto outcome = fixture.send(
        R"({"type":"plan_threading","form":{"pitch":null,"internal":false,"depth":null,)"
        R"("infeed_deg":null,"length":null,"taper_deg":null,"run_out":null,"retract":null,)"
        R"("first_cut":null,"cut_factor":null,"min_cut":null,"spring_cuts":null}})");
    CHECK(contains(outcome.replies, R"("field":"pitch")"));
    CHECK(contains(outcome.replies, R"("field":"length")"));
    CHECK(contains(outcome.replies, R"("backplot":null)"));
}

TEST_CASE("a previewed cycle starts only in canned-cycle mode", "[session][safety]")
{
    Fixture fixture{};
    static_cast<void>(fixture.send(R"({"type":"select_mode","mode":"manual"})"));
    fixture.settle(10.0, 0.0);
    static_cast<void>(fixture.send(thread_m6));
    const auto refused = fixture.send(R"({"type":"start_cycle"})");
    CHECK(!has_run_cycle(refused));
    CHECK(contains(refused.replies, "Canned Cycles"));

    static_cast<void>(fixture.send(R"({"type":"select_mode","mode":"cycle"})"));
    fixture.settle(0.0, 0.0);
    static_cast<void>(fixture.send(thread_m6));
    CHECK(has_run_cycle(fixture.send(R"({"type":"start_cycle"})")));
}

TEST_CASE("a cycle never starts without a fresh preview", "[session][safety]")
{
    Fixture fixture{};
    static_cast<void>(fixture.send(R"({"type":"select_mode","mode":"cycle"})"));
    fixture.settle(0.0, 0.0);
    CHECK(!has_run_cycle(fixture.send(R"({"type":"start_cycle"})")));

    static_cast<void>(fixture.send(thread_m6));
    fixture.settle(0.5, 0.0); // the slide moved after the preview
    const auto moved = fixture.send(R"({"type":"start_cycle"})");
    CHECK(!has_run_cycle(moved));
    CHECK(contains(moved.replies, "moved"));

    static_cast<void>(fixture.send(thread_m6));
    CHECK(has_run_cycle(fixture.send(R"({"type":"start_cycle"})")));
    // Each preview runs once.
    CHECK(!has_run_cycle(fixture.send(R"({"type":"start_cycle"})")));
}

TEST_CASE("changing mode asks LinuxCNC to re-sync motion", "[session]")
{
    Fixture fixture{};
    static_cast<void>(fixture.send(R"({"type":"select_mode","mode":"cycle"})"));
    const auto tick = fixture.tick(10.0, 0.0);
    CHECK((tick.job.has_value() && std::holds_alternative<elle::app::ResyncMotion>(*tick.job)));
}

TEST_CASE("tool and preference changes are saved", "[session]")
{
    Fixture fixture{};
    const auto outcome = fixture.send(R"({"type":"select_tool","id":3})");
    CHECK(contains(outcome.replies, R"("current_tool":3)"));
    const auto loaded = elle::app::load_settings(fixture.settings_file(), "missing.json");
    CHECK(loaded.settings.machine.current_tool == 3);

    static_cast<void>(fixture.send(
        R"({"type":"set_preferences","preferences":{"diameter_mode":false,"metric_on_startup":false,)"
        R"("thread_preset_tab":1,"turning_preset_tab":2,"pitch_preset_tabs":[1,2],)"
        R"("encoder_scale_z":0.002,"encoder_scale_x":-0.002}})"));
    const auto again = elle::app::load_settings(fixture.settings_file(), "missing.json");
    CHECK(!again.settings.diameter_mode);
    CHECK(again.settings.pitch_preset_tabs.at(1) == 2);
    CHECK(again.settings.machine.encoder_scale_z > 0.0019);
}

TEST_CASE("status reports the readout and LinuxCNC state", "[session]")
{
    Fixture fixture{};
    fixture.session().observe({.connected = true, .program_running = true, .error = false});
    const std::string status = fixture.session().status();
    CHECK(status.contains(R"("type":"status")"));
    CHECK(status.contains(R"("program_running":true)"));
    CHECK(status.contains(R"("feed":"longitudinal")"));
}

TEST_CASE("quitting stops any jog", "[session][safety]")
{
    Fixture fixture{};
    static_cast<void>(fixture.send(R"({"type":"select_mode","mode":"manual"})"));
    static_cast<void>(fixture.send(R"({"type":"jog","button":"z_minus","held":true})"));
    CHECK(fixture.tick(10.0, 0.0).outputs.control_z_type);
    CHECK(fixture.send(R"({"type":"quit"})").quit);
    CHECK(!fixture.tick(10.0, 0.0).outputs.control_z_type);
}

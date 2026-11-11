#include <elle/core/jog.hpp>
#include <elle/core/numbers.hpp>

#include <catch2/catch_test_macros.hpp>

#include <chrono>

using elle::core::exactly_equal;
using elle::core::Jog;
using elle::core::JogButton;
using std::chrono::milliseconds;

namespace {

const Jog::Clock::time_point start{};

} // namespace

TEST_CASE("jog speed ramps with hold time up to the axis limit", "[jog]")
{
    Jog jog{};
    jog.hold(JogButton::z_minus, start);
    for (int step = 1; step <= 30; ++step) {
        jog.hold(JogButton::z_minus, start + milliseconds{100 * step});
    }
    const auto ramping = jog.velocities(start + milliseconds{500});
    REQUIRE(ramping.z.has_value());
    CHECK(exactly_equal(*ramping.z, -1.5));
    CHECK(!ramping.x.has_value());

    const auto capped = jog.velocities(start + milliseconds{3000});
    REQUIRE(capped.z.has_value());
    CHECK(exactly_equal(*capped.z, -6.0));
}

TEST_CASE("the cross slide jogs slower than the carriage", "[jog]")
{
    Jog jog{};
    jog.hold(JogButton::x_plus, start);
    jog.hold(JogButton::x_plus, start + milliseconds{5000});
    const auto result = jog.velocities(start + milliseconds{5000});
    REQUIRE(result.x.has_value());
    CHECK(exactly_equal(*result.x, 3.0));
}

TEST_CASE("a button that is no longer refreshed stops by itself", "[jog][safety]")
{
    Jog jog{};
    jog.hold(JogButton::z_plus, start);
    CHECK(jog.velocities(start + Jog::deadman_timeout).z.has_value());
    CHECK(!jog.velocities(start + Jog::deadman_timeout + milliseconds{1}).z.has_value());
    // Expired means released: it does not come back on its own.
    CHECK(!jog.velocities(start + Jog::deadman_timeout + milliseconds{2}).z.has_value());
}

TEST_CASE("releasing stops the axis", "[jog]")
{
    Jog jog{};
    jog.hold(JogButton::x_minus, start);
    jog.release(JogButton::x_minus);
    CHECK(!jog.velocities(start).x.has_value());
}

TEST_CASE("opposing buttons on one axis cancel out", "[jog][safety]")
{
    Jog jog{};
    jog.hold(JogButton::z_minus, start);
    jog.hold(JogButton::z_plus, start);
    CHECK(!jog.velocities(start + milliseconds{10}).z.has_value());
}

TEST_CASE("both axes can jog at once", "[jog]")
{
    Jog jog{};
    jog.hold(JogButton::x_minus, start);
    jog.hold(JogButton::z_plus, start);
    const auto result = jog.velocities(start + milliseconds{100});
    CHECK(result.x.has_value());
    CHECK(result.z.has_value());
    jog.release_all();
    const auto released = jog.velocities(start + milliseconds{100});
    CHECK(!released.x.has_value());
    CHECK(!released.z.has_value());
}

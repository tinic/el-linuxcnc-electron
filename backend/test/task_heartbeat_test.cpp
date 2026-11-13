#include <elle/core/task.hpp>

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cstdint>
#include <limits>

using elle::core::TaskHeartbeat;
using std::chrono::milliseconds;

namespace {

constexpr TaskHeartbeat::Clock::time_point start{};

} // namespace

TEST_CASE("a single heartbeat reading does not prove task is alive", "[task]")
{
    TaskHeartbeat heartbeat{};
    CHECK(!heartbeat.alive(41, start));
}

TEST_CASE("task is alive while its heartbeat advances", "[task]")
{
    TaskHeartbeat heartbeat{};
    CHECK(!heartbeat.alive(41, start));
    CHECK(heartbeat.alive(42, start + milliseconds{50}));
    CHECK(heartbeat.alive(42, start + milliseconds{900}));
    CHECK(heartbeat.alive(43, start + milliseconds{1500}));
}

TEST_CASE("a heartbeat that stops advancing goes stale", "[task]")
{
    TaskHeartbeat heartbeat{};
    CHECK(!heartbeat.alive(7, start));
    CHECK(heartbeat.alive(8, start + milliseconds{10}));
    CHECK(!heartbeat.alive(8, start + milliseconds{10} + TaskHeartbeat::stale_after));
    CHECK(heartbeat.alive(9, start + milliseconds{5000}));
}

TEST_CASE("a status buffer left behind by a dead task never counts as alive", "[task]")
{
    TaskHeartbeat heartbeat{};
    for (int poll = 0; poll < 100; ++poll) {
        CHECK(!heartbeat.alive(1234, start + milliseconds{50 * poll}));
    }
}

TEST_CASE("the heartbeat counter may wrap around", "[task]")
{
    TaskHeartbeat heartbeat{};
    CHECK(!heartbeat.alive(std::numeric_limits<std::int64_t>::max(), start));
    CHECK(heartbeat.alive(std::numeric_limits<std::int64_t>::min(), start + milliseconds{10}));
}

#include <elle/app/task_worker.hpp>
#include <elle/core/task.hpp>

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <expected>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

using elle::core::MachineState;
using elle::core::TaskError;
using elle::core::TaskMode;
using elle::core::TaskStatus;

namespace {

/// Records every command and answers like an idle, enabled LinuxCNC in MDI mode.
class FakeTask final : public elle::core::TaskPort {
public:
    using Result = std::expected<void, TaskError>;

    [[nodiscard]] auto status() -> std::optional<TaskStatus> override
    {
        const std::scoped_lock lock{mutex_};
        return status_;
    }
    auto set_state(MachineState state) -> Result override
    {
        record(state == MachineState::on ? "on" : state == MachineState::off ? "off" : "state");
        return {};
    }
    auto set_mode(TaskMode /*mode*/) -> Result override
    {
        record("mdi mode");
        const std::scoped_lock lock{mutex_};
        status_.mode = TaskMode::mdi;
        return {};
    }
    auto mdi(std::string_view command) -> Result override
    {
        record(std::string{"mdi "} + std::string{command});
        return {};
    }
    auto abort() -> Result override
    {
        record("abort");
        return {};
    }
    auto reset_interpreter() -> Result override
    {
        record("reset");
        return {};
    }
    auto wait_complete(std::chrono::milliseconds /*timeout*/) -> Result override { return {}; }
    [[nodiscard]] auto operator_messages() -> std::vector<std::string> override
    {
        const std::scoped_lock lock{mutex_};
        return std::exchange(messages_, {});
    }

    auto set_status(const TaskStatus& status) -> void
    {
        const std::scoped_lock lock{mutex_};
        status_ = status;
    }
    auto report(std::string message) -> void
    {
        const std::scoped_lock lock{mutex_};
        messages_.push_back(std::move(message));
    }
    [[nodiscard]] auto calls() const -> std::vector<std::string>
    {
        const std::scoped_lock lock{mutex_};
        return calls_;
    }

private:
    auto record(std::string call) -> void
    {
        const std::scoped_lock lock{mutex_};
        calls_.push_back(std::move(call));
    }

    mutable std::mutex mutex_{};
    TaskStatus status_{.state = MachineState::on,
                       .mode = TaskMode::manual,
                       .motion_enabled = true,
                       .interpreter_idle = true};
    std::vector<std::string> calls_{};
    std::vector<std::string> messages_{};
};

/// Waits (up to two seconds) until the worker has logged at least `count` lines.
auto wait_for_log(elle::app::TaskWorker& worker, std::size_t count) -> std::vector<std::string>
{
    std::vector<std::string> lines{};
    for (int attempt = 0; attempt < 200 && lines.size() < count; ++attempt) {
        auto more = worker.take_log();
        lines.insert(lines.end(), more.begin(), more.end());
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
    }
    return lines;
}

auto scratch() -> std::filesystem::path
{
    auto directory = std::filesystem::temp_directory_path() / "elle-task-worker-test";
    std::filesystem::remove_all(directory);
    std::filesystem::create_directories(directory);
    return directory;
}

} // namespace

TEST_CASE("a cycle is written as a subroutine and called through MDI", "[task]")
{
    FakeTask task{};
    const auto directory = scratch();
    elle::app::TaskWorker worker{task, directory};
    worker.submit(elle::app::RunCycle{.program = {"G8", "G0 X1 Z2"}});
    const auto log = wait_for_log(worker, 1);
    REQUIRE(!log.empty());
    CHECK(log.front().starts_with("Cycle started"));

    std::ifstream file{worker.cycle_file()};
    const std::string text{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
    CHECK(text == "o<canned-cycle> sub\nG8\nG0 X1 Z2\no<canned-cycle> endsub\n");

    const auto calls = task.calls();
    REQUIRE(!calls.empty());
    CHECK(calls.front() == "on");
    CHECK(calls.back() == "mdi o<canned-cycle> call");
    // The interpreter is reset right before the call so it reads this cycle's file.
    CHECK(calls.at(calls.size() - 2) == "reset");
}

TEST_CASE("a cycle is not started while LinuxCNC is in E-stop", "[task][safety]")
{
    FakeTask task{};
    task.set_status({.state = MachineState::estop, .motion_enabled = true, .interpreter_idle = true});
    elle::app::TaskWorker worker{task, scratch()};
    worker.submit(elle::app::RunCycle{.program = {"G0 X1"}});
    const auto log = wait_for_log(worker, 1);
    REQUIRE(!log.empty());
    CHECK(log.front().contains("E-stop"));
    for (const auto& call : task.calls()) {
        CHECK(!call.starts_with("mdi o<"));
    }
}

TEST_CASE("resync turns the machine off and on, then resets the interpreter", "[task]")
{
    FakeTask task{};
    elle::app::TaskWorker worker{task, scratch()};
    worker.submit(elle::app::ResyncMotion{});
    worker.submit(elle::app::AbortTask{});
    static_cast<void>(wait_for_log(worker, 1)); // "Cycle stopped" from the abort that follows
    const auto calls = task.calls();
    const std::vector<std::string> expected{"mdi mode", "off", "on", "reset", "abort"};
    CHECK(calls == expected);
}

TEST_CASE("LinuxCNC's operator messages reach the log", "[task]")
{
    FakeTask task{};
    elle::app::TaskWorker worker{task, scratch()};
    task.report("joint 1 following error");
    const auto log = wait_for_log(worker, 1);
    REQUIRE(!log.empty());
    CHECK(log.front() == "LinuxCNC: joint 1 following error");
}

TEST_CASE("the snapshot follows task status", "[task]")
{
    FakeTask task{};
    task.set_status({.state = MachineState::on,
                     .mode = TaskMode::mdi,
                     .motion_enabled = true,
                     .interpreter_idle = false});
    elle::app::TaskWorker worker{task, scratch()};
    std::this_thread::sleep_for(std::chrono::milliseconds{150});
    const auto snapshot = worker.snapshot();
    CHECK(snapshot.connected);
    CHECK(snapshot.program_running);
    CHECK(!snapshot.error);
}

#pragma once

#include <elle/core/cycle.hpp>
#include <elle/core/task.hpp>

#include <condition_variable>
#include <deque>
#include <filesystem>
#include <mutex>
#include <optional>
#include <stop_token>
#include <string>
#include <thread>
#include <variant>
#include <vector>

namespace elle::app {

/// Re-synchronize LinuxCNC's commanded position after the encoders were zeroed.
struct ResyncMotion {};

/// Run a generated cycle as an MDI subroutine call.
struct RunCycle {
    core::GcodeLines program{};
};

/// Stop whatever task is doing (not an E-stop).
struct AbortTask {};

using TaskJob = std::variant<ResyncMotion, RunCycle, AbortTask>;

/// What the panel shows about LinuxCNC.
struct TaskSnapshot {
    bool connected{};
    bool program_running{};
    bool error{};
};

/// Runs LinuxCNC command sequences on their own thread, so blocking waits never stall the control
/// loop or the user interface, and polls task status and operator messages in between.
class TaskWorker {
public:
    /// `port` must outlive the worker. Cycle subroutines are written to `cycle_directory`, which
    /// must be on the interpreter's subroutine path (the configuration directory is).
    TaskWorker(core::TaskPort& port, std::filesystem::path cycle_directory);
    TaskWorker(const TaskWorker&) = delete;
    TaskWorker(TaskWorker&&) = delete;
    auto operator=(const TaskWorker&) -> TaskWorker& = delete;
    auto operator=(TaskWorker&&) -> TaskWorker& = delete;
    ~TaskWorker();

    auto submit(TaskJob job) -> void;
    [[nodiscard]] auto snapshot() const -> TaskSnapshot;
    /// Log lines produced since the last call.
    [[nodiscard]] auto take_log() -> std::vector<std::string>;

    /// The subroutine file a cycle is written to.
    [[nodiscard]] auto cycle_file() const -> std::filesystem::path;

private:
    auto run(const std::stop_token& stop) -> void;
    auto execute(const TaskJob& job) -> void;
    auto resync() -> void;
    auto run_cycle(const core::GcodeLines& program) -> void;
    auto abort_task() -> void;
    auto poll() -> void;
    auto log(std::string line) -> void;
    [[nodiscard]] auto ready_for_mdi() -> bool;

    core::TaskPort& port_;
    std::filesystem::path cycle_directory_{};
    mutable std::mutex mutex_{};
    std::condition_variable_any wake_{};
    std::deque<TaskJob> jobs_{};
    TaskSnapshot snapshot_{};
    std::vector<std::string> log_{};
    std::jthread thread_{};
};

} // namespace elle::app

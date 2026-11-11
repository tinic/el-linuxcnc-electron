#pragma once

#include <elle/app/protocol.hpp>
#include <elle/app/settings.hpp>
#include <elle/app/task_worker.hpp>
#include <elle/core/cycle.hpp>
#include <elle/core/lathe_controller.hpp>

#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace elle::app {

/// What handling one panel message asks of the caller.
struct Outcome {
    std::vector<std::string> replies{}; ///< messages for the panel
    std::vector<TaskJob> jobs{};        ///< LinuxCNC work for the task worker
    bool quit{};
};

/// The machine side of the operator panel: applies panel commands to the lathe controller, plans
/// cycles, persists settings, and reports status. Thread-safe: the UI thread calls `handle`, the
/// control loop calls `tick`.
class Session {
public:
    using Clock = core::LatheController::Clock;

    Session(Settings settings, std::filesystem::path settings_file);

    [[nodiscard]] auto handle(std::string_view json, Clock::time_point now) -> Outcome;

    struct Tick {
        core::PinOutputs outputs;
        std::optional<TaskJob> job;
    };
    [[nodiscard]] auto tick(const core::MachineSample& sample, Clock::time_point now) -> Tick;

    /// Latest LinuxCNC state, as polled by the task worker.
    auto observe(const TaskSnapshot& task) -> void;
    [[nodiscard]] auto status() const -> std::string;

private:
    /// A previewed cycle waiting for the operator to run it.
    struct Pending {
        core::GcodeLines program;
        core::WorkPosition planned_at;
    };

    auto apply(const Command& command, Clock::time_point now, Outcome& outcome) -> void;
    auto plan(CycleKind kind, const core::ParameterIssues& issues,
              const std::optional<core::GcodeLines>& machine,
              const std::optional<core::GcodeLines>& preview, std::string error,
              const core::WorkPosition& start) -> CyclePlan;
    auto start_cycle(Outcome& outcome) -> void;
    auto save(Outcome& outcome) -> void;

    mutable std::mutex mutex_{};
    Settings settings_{};
    std::filesystem::path settings_file_{};
    core::LatheController controller_;
    std::optional<Pending> pending_{};
    TaskSnapshot task_{};
};

} // namespace elle::app

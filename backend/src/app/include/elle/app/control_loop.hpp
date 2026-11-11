#pragma once

#include <elle/app/session.hpp>
#include <elle/app/task_worker.hpp>
#include <elle/core/lathe_controller.hpp>
#include <elle/core/task.hpp>

#include <chrono>
#include <expected>
#include <mutex>
#include <optional>
#include <stop_token>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace elle::app {

/// The machine's pins as the control loop sees them.
class MachineIo {
public:
    MachineIo() = default;
    virtual ~MachineIo() = default;
    [[nodiscard]] virtual auto read() -> core::MachineSample = 0;
    virtual auto write(const core::PinOutputs& outputs) -> void = 0;

protected:
    MachineIo(const MachineIo&) = default;
    MachineIo(MachineIo&&) = default;
    auto operator=(const MachineIo&) -> MachineIo& = default;
    auto operator=(MachineIo&&) -> MachineIo& = default;
};

/// Runs the session's control step at a fixed rate on its own thread: read pins, tick, write pins.
class ControlLoop {
public:
    static constexpr auto period = std::chrono::milliseconds{10};

    ControlLoop(Session& session, MachineIo& machine, TaskWorker& worker);
    ControlLoop(const ControlLoop&) = delete;
    ControlLoop(ControlLoop&&) = delete;
    auto operator=(const ControlLoop&) -> ControlLoop& = delete;
    auto operator=(ControlLoop&&) -> ControlLoop& = delete;
    ~ControlLoop();

private:
    auto run(const std::stop_token& stop) -> void;

    Session& session_;
    MachineIo& machine_;
    TaskWorker& worker_;
    std::jthread thread_{};
};

/// A lathe that exists only in memory: jogging moves the slides, the leadscrew follows a spindle
/// turning at a steady speed while engaged. For `elle --demo`.
class SimulatedMachine final : public MachineIo {
public:
    [[nodiscard]] auto read() -> core::MachineSample override;
    auto write(const core::PinOutputs& outputs) -> void override;

private:
    std::mutex mutex_{};
    /// Where the simulated slides start: a 25 mm bar, the tool 3 mm into it.
    static constexpr core::MachineSample start_position{.position_z = -3.0, .position_x = 12.5};
    core::MachineSample sample_{start_position};
    std::optional<std::chrono::steady_clock::time_point> last_{};
    core::PinOutputs outputs_{};
};

/// Stands in for LinuxCNC in demo mode: always ready, runs nothing, says so in the log.
class DemoTask final : public core::TaskPort {
public:
    [[nodiscard]] auto status() -> std::optional<core::TaskStatus> override;
    auto set_state(core::MachineState state) -> std::expected<void, core::TaskError> override;
    auto set_mode(core::TaskMode mode) -> std::expected<void, core::TaskError> override;
    auto mdi(std::string_view command) -> std::expected<void, core::TaskError> override;
    auto abort() -> std::expected<void, core::TaskError> override;
    auto reset_interpreter() -> std::expected<void, core::TaskError> override;
    auto wait_complete(std::chrono::milliseconds timeout)
        -> std::expected<void, core::TaskError> override;
    [[nodiscard]] auto operator_messages() -> std::vector<std::string> override;

private:
    std::mutex mutex_{};
    std::vector<std::string> messages_{};
};

} // namespace elle::app

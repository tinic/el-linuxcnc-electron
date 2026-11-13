#pragma once

#include <chrono>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace elle::core {

enum class MachineState : std::uint8_t {
    estop,
    estop_reset,
    off,
    on,
};

enum class TaskMode : std::uint8_t {
    manual,
    automatic,
    mdi,
};

/// The parts of LinuxCNC's task status Elle acts on.
struct TaskStatus {
    MachineState state{MachineState::estop};
    TaskMode mode{TaskMode::manual};
    bool motion_enabled{};
    bool interpreter_idle{};
    bool waiting_for_motion_or_io{};
    bool execution_error{};
    int call_level{};

    /// A canned cycle (or any program) is being interpreted or executed.
    [[nodiscard]] auto program_running() const noexcept -> bool
    {
        return !interpreter_idle || waiting_for_motion_or_io || call_level > 0;
    }

    [[nodiscard]] auto in_error() const noexcept -> bool
    {
        return state == MachineState::estop || execution_error;
    }
};

/// Tells a live LinuxCNC task from a stale status buffer. Task writes its status every cycle, and
/// each write advances the status buffer's NML write counter; when task has died but linuxcncsvr
/// still serves the shared memory, the last status stays readable forever. (EMC_STAT's own
/// task.heartbeat is not maintained by LinuxCNC 2.9 and stays 0.) Task counts as alive only while
/// that counter keeps advancing.
class TaskHeartbeat {
public:
    using Clock = std::chrono::steady_clock;
    /// Task cycles every few milliseconds; a second without a beat means it is gone or hung.
    static constexpr auto stale_after = std::chrono::milliseconds{1000};

    /// Records a reading of the write counter. The first reading proves nothing, so it is not
    /// alive.
    [[nodiscard]] auto alive(std::int64_t writes, Clock::time_point now) noexcept -> bool;

private:
    std::optional<std::int64_t> last_{};
    Clock::time_point last_change_{};
    bool advanced_{};
};

enum class TaskError : std::uint8_t {
    channel_invalid,
    write_failed,
    not_acknowledged, ///< task did not echo the command within the timeout
    failed,           ///< task finished the command with an error
    timed_out,        ///< task did not finish the command within the timeout
    command_too_long,
};

[[nodiscard]] auto describe(TaskError error) noexcept -> std::string_view;

/// LinuxCNC's task controller as Elle uses it. Commands return once task has received them;
/// `wait_complete` blocks until task has finished the most recent one.
class TaskPort {
public:
    static constexpr auto default_timeout = std::chrono::milliseconds{5000};

    TaskPort() = default;
    virtual ~TaskPort() = default;

    [[nodiscard]] virtual auto status() -> std::optional<TaskStatus> = 0;
    virtual auto set_state(MachineState state) -> std::expected<void, TaskError> = 0;
    virtual auto set_mode(TaskMode mode) -> std::expected<void, TaskError> = 0;
    virtual auto mdi(std::string_view command) -> std::expected<void, TaskError> = 0;
    virtual auto abort() -> std::expected<void, TaskError> = 0;
    virtual auto reset_interpreter() -> std::expected<void, TaskError> = 0;
    virtual auto wait_complete(std::chrono::milliseconds timeout)
        -> std::expected<void, TaskError> = 0;
    /// Operator errors and messages LinuxCNC reported since the last call.
    [[nodiscard]] virtual auto operator_messages() -> std::vector<std::string> = 0;

protected:
    TaskPort(const TaskPort&) = default;
    TaskPort(TaskPort&&) = default;
    auto operator=(const TaskPort&) -> TaskPort& = default;
    auto operator=(TaskPort&&) -> TaskPort& = default;
};

} // namespace elle::core

#include <elle/linuxcnc/task_client.hpp>

#include <elle/core/task.hpp>

#include <algorithm>
#include <chrono>
#include <expected>
#include <filesystem>
#include <format>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include <cmd_msg.hh>
#include <cms.hh>
#include <emc.hh>
#include <emc_nml.hh>
#include <nml.hh>
#include <nml_type.hh>
#include <rcs.hh>
#include <stat_msg.hh>

namespace elle::linuxcnc {

namespace {

using Clock = std::chrono::steady_clock;

/// Interval between status polls while waiting, as in LinuxCNC's own Python binding.
constexpr auto poll_interval = std::chrono::milliseconds{10};

auto to_emc(core::MachineState state) -> EMC_TASK_STATE_ENUM
{
    switch (state) {
    case core::MachineState::estop:
        return EMC_TASK_STATE_ESTOP;
    case core::MachineState::estop_reset:
        return EMC_TASK_STATE_ESTOP_RESET;
    case core::MachineState::off:
        return EMC_TASK_STATE_OFF;
    case core::MachineState::on:
    default:
        return EMC_TASK_STATE_ON;
    }
}

auto from_emc(EMC_TASK_STATE_ENUM state) -> core::MachineState
{
    switch (state) {
    case EMC_TASK_STATE_ESTOP_RESET:
        return core::MachineState::estop_reset;
    case EMC_TASK_STATE_OFF:
        return core::MachineState::off;
    case EMC_TASK_STATE_ON:
        return core::MachineState::on;
    case EMC_TASK_STATE_ESTOP:
    default:
        return core::MachineState::estop;
    }
}

auto to_emc(core::TaskMode mode) -> EMC_TASK_MODE_ENUM
{
    switch (mode) {
    case core::TaskMode::manual:
        return EMC_TASK_MODE_MANUAL;
    case core::TaskMode::automatic:
        return EMC_TASK_MODE_AUTO;
    case core::TaskMode::mdi:
    default:
        return EMC_TASK_MODE_MDI;
    }
}

auto from_emc(EMC_TASK_MODE_ENUM mode) -> core::TaskMode
{
    switch (mode) {
    case EMC_TASK_MODE_AUTO:
        return core::TaskMode::automatic;
    case EMC_TASK_MODE_MDI:
        return core::TaskMode::mdi;
    case EMC_TASK_MODE_MANUAL:
    default:
        return core::TaskMode::manual;
    }
}

auto waiting_for_motion_or_io(EMC_TASK_EXEC_ENUM state) -> bool
{
    return state == EMC_TASK_EXEC_WAITING_FOR_MOTION ||
           state == EMC_TASK_EXEC_WAITING_FOR_MOTION_QUEUE ||
           state == EMC_TASK_EXEC_WAITING_FOR_IO;
}

/// Text of a fixed-size, NUL-terminated NML string field.
auto text_of(std::span<const char> field) -> std::string
{
    const auto end = std::ranges::find(field, '\0');
    return {field.begin(), end};
}

} // namespace

struct TaskClient::Channels {
    std::unique_ptr<RCS_CMD_CHANNEL> command{};
    std::unique_ptr<RCS_STAT_CHANNEL> status{};
    std::unique_ptr<NML> errors{};
    core::TaskHeartbeat heartbeat{};
    int serial{};
    bool have_status{}; ///< the channel's buffer holds a task status from an earlier peek

    /// The latest task status. peek() returns 0 when task has written nothing since the last
    /// peek (it does so every cycle, so a caller asking twice within one cycle sees 0); the
    /// channel's buffer then still holds the previous status, which is current. As LinuxCNC's
    /// own Python module does, that status is used rather than reported as missing. Whether
    /// task is still alive is the heartbeat's job (the write counter stops advancing).
    [[nodiscard]] auto latest_status() -> EMC_STAT*
    {
        const NMLTYPE type = status->peek();
        if (type == EMC_STAT_TYPE) {
            have_status = true;
        } else if (type != 0) {
            return nullptr; // an error, or a message that is not a task status
        }
        if (!have_status) {
            return nullptr;
        }
        // NML hands out the base class; a task status was seen in this buffer (type tag above).
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
        return static_cast<EMC_STAT*>(status->get_address());
    }

    /// Sends a command and waits until task has received it (echoed its serial number).
    auto send(RCS_CMD_MSG& message) -> std::expected<void, core::TaskError>
    {
        if (command->write(&message) != 0) {
            return std::unexpected{core::TaskError::write_failed};
        }
        serial = message.serial_number;
        const auto deadline = Clock::now() + core::TaskPort::default_timeout;
        while (Clock::now() < deadline) {
            if (const EMC_STAT* stat = latest_status();
                stat != nullptr && stat->echo_serial_number - serial >= 0) {
                return {};
            }
            std::this_thread::sleep_for(poll_interval);
        }
        return std::unexpected{core::TaskError::not_acknowledged};
    }
};

auto TaskClient::connect(const std::filesystem::path& nml_file)
    -> std::expected<TaskClient, std::string>
{
    auto channels = std::make_unique<Channels>();
    channels->command = std::make_unique<RCS_CMD_CHANNEL>(emcFormat, "emcCommand", "xemc",
                                                          nml_file.c_str());
    channels->status = std::make_unique<RCS_STAT_CHANNEL>(emcFormat, "emcStatus", "xemc",
                                                          nml_file.c_str());
    channels->errors = std::make_unique<NML>(emcFormat, "emcError", "xemc", nml_file.c_str());
    if (channels->command->valid() == 0 || channels->status->valid() == 0 ||
        channels->errors->valid() == 0) {
        return std::unexpected{
            std::format("cannot open the LinuxCNC NML channels from {}", nml_file.string())};
    }
    return TaskClient{std::move(channels)};
}

TaskClient::TaskClient(std::unique_ptr<Channels> channels) noexcept
    : channels_{std::move(channels)}
{
}

TaskClient::TaskClient(TaskClient&& other) noexcept = default;
auto TaskClient::operator=(TaskClient&& other) noexcept -> TaskClient& = default;
TaskClient::~TaskClient() = default;

auto TaskClient::status() -> std::optional<core::TaskStatus>
{
    const EMC_STAT* stat = channels_->latest_status();
    // linuxcncsvr keeps serving the last status after task has died; that is not a connection.
    // The buffer's write counter advances with every status task writes (once per task cycle).
    if (stat == nullptr ||
        !channels_->heartbeat.alive(channels_->status->cms->header.write_id, Clock::now())) {
        return std::nullopt;
    }
    return core::TaskStatus{
        .state = from_emc(stat->task.state),
        .mode = from_emc(stat->task.mode),
        .motion_enabled = stat->motion.traj.enabled,
        .interpreter_idle = stat->task.interpState == EMC_TASK_INTERP_IDLE,
        .waiting_for_motion_or_io = waiting_for_motion_or_io(stat->task.execState),
        .execution_error = stat->task.execState == EMC_TASK_EXEC_ERROR,
        .call_level = stat->task.callLevel,
    };
}

auto TaskClient::set_state(core::MachineState state) -> std::expected<void, core::TaskError>
{
    EMC_TASK_SET_STATE message{};
    message.state = to_emc(state);
    return channels_->send(message);
}

auto TaskClient::set_mode(core::TaskMode mode) -> std::expected<void, core::TaskError>
{
    EMC_TASK_SET_MODE message{};
    message.mode = to_emc(mode);
    return channels_->send(message);
}

auto TaskClient::mdi(std::string_view command) -> std::expected<void, core::TaskError>
{
    EMC_TASK_PLAN_EXECUTE message{};
    const std::span<char> buffer{message.command};
    if (command.size() >= buffer.size()) {
        return std::unexpected{core::TaskError::command_too_long};
    }
    std::ranges::fill(buffer, '\0');
    std::ranges::copy(command, buffer.begin());
    return channels_->send(message);
}

auto TaskClient::abort() -> std::expected<void, core::TaskError>
{
    EMC_TASK_ABORT message{};
    return channels_->send(message);
}

auto TaskClient::reset_interpreter() -> std::expected<void, core::TaskError>
{
    EMC_TASK_PLAN_INIT message{};
    return channels_->send(message);
}

auto TaskClient::wait_complete(std::chrono::milliseconds timeout) -> std::expected<void, core::TaskError>
{
    const auto deadline = Clock::now() + timeout;
    while (Clock::now() < deadline) {
        if (const EMC_STAT* stat = channels_->latest_status(); stat != nullptr) {
            const int serial_diff = stat->echo_serial_number - channels_->serial;
            if (serial_diff > 0) {
                return {};
            }
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access): RCS_STAT_MSG's own union.
            const int state = stat->status; // an int in LinuxCNC 2.9
            if (serial_diff == 0 && state == RCS_DONE) {
                return {};
            }
            if (serial_diff == 0 && state == RCS_ERROR) {
                return std::unexpected{core::TaskError::failed};
            }
        }
        std::this_thread::sleep_for(poll_interval);
    }
    return std::unexpected{core::TaskError::timed_out};
}

auto TaskClient::operator_messages() -> std::vector<std::string>
{
    std::vector<std::string> messages{};
    for (NMLTYPE type = channels_->errors->read(); type > 0; type = channels_->errors->read()) {
        // NML hands out the base class; each downcast below follows its own type tag.
        NMLmsg* message = channels_->errors->get_address();
        if (type == EMC_OPERATOR_ERROR_TYPE) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast): tag checked
            messages.push_back(text_of(static_cast<EMC_OPERATOR_ERROR*>(message)->error));
        } else if (type == EMC_OPERATOR_TEXT_TYPE) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast): tag checked
            messages.push_back(text_of(static_cast<EMC_OPERATOR_TEXT*>(message)->text));
        } else if (type == EMC_OPERATOR_DISPLAY_TYPE) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast): tag checked
            messages.push_back(text_of(static_cast<EMC_OPERATOR_DISPLAY*>(message)->display));
        }
    }
    return messages;
}

} // namespace elle::linuxcnc

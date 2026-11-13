#include <elle/core/task.hpp>

#include <cstdint>
#include <string_view>

namespace elle::core {

auto describe(TaskError error) noexcept -> std::string_view
{
    switch (error) {
    case TaskError::channel_invalid:
        return "the NML channel to LinuxCNC is not valid";
    case TaskError::write_failed:
        return "sending the command to LinuxCNC failed";
    case TaskError::not_acknowledged:
        return "LinuxCNC did not acknowledge the command";
    case TaskError::failed:
        return "LinuxCNC reported an error executing the command";
    case TaskError::timed_out:
        return "LinuxCNC did not finish the command in time";
    case TaskError::command_too_long:
        return "the MDI command is too long";
    default:
        return "unknown task error";
    }
}

auto TaskHeartbeat::alive(std::uint32_t heartbeat, Clock::time_point now) noexcept -> bool
{
    if (last_ != heartbeat) {
        advanced_ = last_.has_value();
        last_ = heartbeat;
        last_change_ = now;
        return advanced_;
    }
    return advanced_ && now - last_change_ < stale_after;
}

} // namespace elle::core

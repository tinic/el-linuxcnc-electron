#pragma once

#include <elle/core/task.hpp>

#include <chrono>
#include <expected>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace elle::linuxcnc {

/// LinuxCNC's task controller over NML (command, status and error channels), with the same
/// semantics as the `linuxcnc` Python module.
class TaskClient final : public core::TaskPort {
public:
    [[nodiscard]] static auto connect(const std::filesystem::path& nml_file)
        -> std::expected<TaskClient, std::string>;

    TaskClient(const TaskClient&) = delete;
    auto operator=(const TaskClient&) -> TaskClient& = delete;
    TaskClient(TaskClient&& other) noexcept;
    auto operator=(TaskClient&& other) noexcept -> TaskClient&;
    ~TaskClient() override;

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
    struct Channels;
    explicit TaskClient(std::unique_ptr<Channels> channels) noexcept;

    std::unique_ptr<Channels> channels_{};
};

} // namespace elle::linuxcnc

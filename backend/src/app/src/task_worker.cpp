#include <elle/app/task_worker.hpp>

#include <elle/core/cycle.hpp>
#include <elle/core/task.hpp>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <iterator>
#include <mutex>
#include <optional>
#include <stop_token>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace elle::app {

namespace {

constexpr auto poll_interval = std::chrono::milliseconds{50};
constexpr auto mode_change_pause = std::chrono::milliseconds{100};
constexpr int mode_change_attempts = 50;
constexpr std::string_view subroutine = "canned-cycle";


} // namespace

TaskWorker::TaskWorker(core::TaskPort& port, std::filesystem::path cycle_directory)
    : port_{port}
    , cycle_directory_{std::move(cycle_directory)}
    , thread_{[this](const std::stop_token& stop) { run(stop); }}
{
}

TaskWorker::~TaskWorker()
{
    thread_.request_stop();
    wake_.notify_all();
}

auto TaskWorker::submit(TaskJob job) -> void
{
    {
        const std::scoped_lock<std::mutex> lock{mutex_};
        jobs_.push_back(std::move(job));
    }
    wake_.notify_all();
}

auto TaskWorker::snapshot() const -> TaskSnapshot
{
    const std::scoped_lock<std::mutex> lock{mutex_};
    return snapshot_;
}

auto TaskWorker::take_log() -> std::vector<std::string>
{
    const std::scoped_lock<std::mutex> lock{mutex_};
    return std::exchange(log_, {});
}

auto TaskWorker::cycle_file() const -> std::filesystem::path
{
    return cycle_directory_ / std::format("{}.ngc", subroutine);
}

auto TaskWorker::run(const std::stop_token& stop) -> void
{
    while (!stop.stop_requested()) {
        std::optional<TaskJob> job;
        {
            std::unique_lock<std::mutex> lock{mutex_};
            wake_.wait_for(lock, stop, poll_interval, [this] { return !jobs_.empty(); });
            if (!jobs_.empty()) {
                job = std::move(jobs_.front());
                jobs_.pop_front();
            }
        }
        if (job.has_value()) {
            execute(*job);
        }
        poll();
    }
}

auto TaskWorker::execute(const TaskJob& job) -> void
{
    std::visit(
        [this](const auto& step) {
            using Step = std::remove_cvref_t<decltype(step)>;
            if constexpr (std::is_same_v<Step, ResyncMotion>) {
                resync();
            } else if constexpr (std::is_same_v<Step, RunCycle>) {
                run_cycle(step.program);
            } else {
                abort_task();
            }
        },
        job);
}

auto TaskWorker::poll() -> void
{
    const auto status = port_.status();
    auto messages = port_.operator_messages();
    const std::scoped_lock<std::mutex> lock{mutex_};
    snapshot_ = {.connected = status.has_value(),
                 .program_running = status.has_value() && status->program_running(),
                 .error = status.has_value() && status->in_error()};
    std::ranges::transform(messages, std::back_inserter(log_), [](const std::string& message) {
        return std::format("LinuxCNC: {}", message);
    });
}

auto TaskWorker::log(std::string line) -> void
{
    const std::scoped_lock<std::mutex> lock{mutex_};
    log_.push_back(std::move(line));
}

/// Switches task to MDI and checks it can take commands, as the original backend did before every
/// cycle and resync: not in E-stop, motion enabled, interpreter idle.
auto TaskWorker::ready_for_mdi() -> bool
{
    for (int attempt = 0; attempt < mode_change_attempts; ++attempt) {
        const auto status = port_.status();
        if (!status.has_value()) {
            log("LinuxCNC is not reporting status");
            return false;
        }
        if (status->state == core::MachineState::estop) {
            log("LinuxCNC is in E-stop");
            return false;
        }
        if (!status->motion_enabled) {
            log("LinuxCNC motion is not enabled");
            return false;
        }
        if (!status->interpreter_idle) {
            log("LinuxCNC's interpreter is busy");
            return false;
        }
        if (status->mode == core::TaskMode::mdi) {
            return true;
        }
        if (!succeeded(port_.set_mode(core::TaskMode::mdi), "switching to MDI")) {
            return false;
        }
        std::this_thread::sleep_for(mode_change_pause);
    }
    log("LinuxCNC did not switch to MDI");
    return false;
}

auto TaskWorker::succeeded(const Result& result, std::string_view step) -> bool
{
    if (result.has_value()) {
        return true;
    }
    log(std::format("{}: {}", step, core::describe(result.error())));
    return false;
}

auto TaskWorker::finished(std::string_view step) -> bool
{
    return succeeded(port_.wait_complete(core::TaskPort::default_timeout), step);
}

auto TaskWorker::resync() -> void
{
    if (!succeeded(port_.set_mode(core::TaskMode::mdi), "switching to MDI") ||
        !finished("waiting for MDI") || !ready_for_mdi()) {
        return;
    }
    // Machine off and on again makes motion take the zeroed encoder positions as its commanded
    // position; then the interpreter starts from a clean state. Each step needs the previous one.
    static_cast<void>(succeeded(port_.set_state(core::MachineState::off), "machine off") &&
                      finished("waiting for machine off") &&
                      succeeded(port_.set_state(core::MachineState::on), "machine on") &&
                      finished("waiting for machine on") &&
                      succeeded(port_.reset_interpreter(), "interpreter reset") &&
                      finished("waiting for interpreter reset"));
}

auto TaskWorker::write_cycle(const core::GcodeLines& program) -> bool
{
    std::ofstream file{cycle_file(), std::ios::trunc};
    file << std::format("o<{}> sub\n", subroutine);
    for (const auto& line : program) {
        file << line << '\n';
    }
    file << std::format("o<{}> endsub\n", subroutine);
    if (!file.flush()) {
        log(std::format("writing {} failed", cycle_file().string()));
        return false;
    }
    return true;
}

auto TaskWorker::run_cycle(const core::GcodeLines& program) -> void
{
    // The interpreter caches subroutine files; it is reset after writing so this cycle's file is
    // read fresh.
    const bool started = succeeded(port_.set_state(core::MachineState::on), "machine on") &&
                         finished("waiting for machine on") && ready_for_mdi() &&
                         write_cycle(program) &&
                         succeeded(port_.reset_interpreter(), "interpreter reset") &&
                         finished("waiting for interpreter reset") &&
                         succeeded(port_.mdi(std::format("o<{}> call", subroutine)), "starting");
    if (started) {
        log(std::format("Cycle started ({} lines)", program.size()));
    }
}

auto TaskWorker::abort_task() -> void
{
    static_cast<void>(succeeded(port_.abort(), "abort"));
    std::error_code ignored{};
    std::filesystem::remove(cycle_file(), ignored);
    log("Cycle stopped");
}

} // namespace elle::app

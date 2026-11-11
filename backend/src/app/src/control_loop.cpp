#include <elle/app/control_loop.hpp>

#include <elle/app/session.hpp>
#include <elle/app/task_worker.hpp>
#include <elle/core/lathe_controller.hpp>
#include <elle/core/task.hpp>

#include <chrono>
#include <expected>
#include <format>
#include <mutex>
#include <optional>
#include <stop_token>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace elle::app {

namespace {

/// Spindle speed of the simulated lathe, revolutions per second.
constexpr double demo_spindle_rps = 5.0;

} // namespace

ControlLoop::ControlLoop(Session& session, MachineIo& machine, TaskWorker& worker)
    : session_{session}
    , machine_{machine}
    , worker_{worker}
    , thread_{[this](const std::stop_token& stop) { run(stop); }}
{
}

ControlLoop::~ControlLoop()
{
    thread_.request_stop();
}

auto ControlLoop::run(const std::stop_token& stop) -> void
{
    auto next = Session::Clock::now();
    while (!stop.stop_requested()) {
        const auto now = Session::Clock::now();
        auto tick = session_.tick(machine_.read(), now);
        machine_.write(tick.outputs);
        if (tick.job.has_value()) {
            worker_.submit(std::move(*tick.job));
        }
        next += period;
        if (next < now) {
            next = now; // fell behind (e.g. suspended); do not try to catch up in a burst
        }
        std::this_thread::sleep_until(next);
    }
}

auto SimulatedMachine::read() -> core::MachineSample
{
    const std::scoped_lock lock{mutex_};
    const auto now = std::chrono::steady_clock::now();
    if (last_.has_value()) {
        const double seconds = std::chrono::duration<double>(now - *last_).count();
        const double revolutions = demo_spindle_rps * seconds;
        sample_.spindle_revs += revolutions;
        sample_.spindle_rps = demo_spindle_rps;
        if (outputs_.control_x_type) {
            sample_.position_x += outputs_.velocity_x_cmd * seconds;
        } else if (outputs_.enable_stepper_x && !outputs_.control_source) {
            sample_.position_x += outputs_.forward_x * revolutions;
        }
        if (outputs_.control_z_type) {
            sample_.position_z += outputs_.velocity_z_cmd * seconds;
        } else if (outputs_.enable_stepper_z && !outputs_.control_source) {
            sample_.position_z += outputs_.forward_z * revolutions;
        }
        sample_.stepper_x = sample_.position_x;
        sample_.stepper_z = sample_.position_z;
    }
    last_ = now;
    return sample_;
}

auto SimulatedMachine::write(const core::PinOutputs& outputs) -> void
{
    const std::scoped_lock lock{mutex_};
    if (outputs.reset_x != outputs_.reset_x) {
        sample_.position_x = 0.0;
    }
    if (outputs.reset_z != outputs_.reset_z) {
        sample_.position_z = 0.0;
    }
    outputs_ = outputs;
}

auto DemoTask::status() -> std::optional<core::TaskStatus>
{
    return core::TaskStatus{.state = core::MachineState::on,
                            .mode = core::TaskMode::mdi,
                            .motion_enabled = true,
                            .interpreter_idle = true};
}

auto DemoTask::set_state(core::MachineState /*state*/) -> std::expected<void, core::TaskError>
{
    return {};
}

auto DemoTask::set_mode(core::TaskMode /*mode*/) -> std::expected<void, core::TaskError>
{
    return {};
}

auto DemoTask::mdi(std::string_view command) -> std::expected<void, core::TaskError>
{
    const std::scoped_lock lock{mutex_};
    messages_.push_back(std::format("demo mode, not run: {}", command));
    return {};
}

auto DemoTask::abort() -> std::expected<void, core::TaskError>
{
    return {};
}

auto DemoTask::reset_interpreter() -> std::expected<void, core::TaskError>
{
    return {};
}

auto DemoTask::wait_complete(std::chrono::milliseconds /*timeout*/)
    -> std::expected<void, core::TaskError>
{
    return {};
}

auto DemoTask::operator_messages() -> std::vector<std::string>
{
    const std::scoped_lock lock{mutex_};
    return std::exchange(messages_, {});
}

} // namespace elle::app

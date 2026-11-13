#include <elle/core/jog.hpp>
#include <elle/core/lathe_controller.hpp>
#include <elle/core/leadscrew.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cstdint>

using Catch::Approx;
using elle::core::Axis;
using elle::core::DirectionMode;
using elle::core::FeedMode;
using elle::core::JogButton;
using elle::core::LatheController;
using elle::core::MachineSample;
using elle::core::OperatingMode;
using elle::core::TickResult;
using std::chrono::milliseconds;

namespace {

constexpr int reference_tool = 0;
constexpr int cutoff_tool = 1;

auto settings_with_tools() -> elle::core::MachineSettings
{
    return {.tools = {{.id = reference_tool, .description = "Reference"},
                      {.id = cutoff_tool, .offset_x = -7.95, .offset_z = 0.25,
                       .description = "Cutoff"}},
            .current_tool = cutoff_tool};
}

/// Where the slides' encoders read, mm.
struct Slides {
    double x{};
    double z{};
};

/// Drives a controller tick by tick, simulating the encoder component honouring resets.
class Bench {
public:
    explicit Bench(LatheController& controller) : controller_{&controller} {}

    auto move_to(Slides slides) -> void
    {
        sample_.position_x = slides.x;
        sample_.position_z = slides.z;
    }

    auto tick() -> TickResult
    {
        now_ += milliseconds{10};
        const TickResult result = controller_->tick(sample_, now_);
        if (result.outputs.reset_x != reset_x_) {
            reset_x_ = result.outputs.reset_x;
            sample_.position_x = 0.0;
        }
        if (result.outputs.reset_z != reset_z_) {
            reset_z_ = result.outputs.reset_z;
            sample_.position_z = 0.0;
        }
        return result;
    }

    auto settle() -> void
    {
        for (int step = 0; step < 5; ++step) {
            static_cast<void>(tick());
        }
    }

    [[nodiscard]] auto now() const -> LatheController::Clock::time_point { return now_; }
    auto sample() -> MachineSample& { return sample_; }

private:
    LatheController* controller_;
    MachineSample sample_{};
    LatheController::Clock::time_point now_{};
    std::uint32_t reset_x_{};
    std::uint32_t reset_z_{};
};

/// Switches mode and checks over the following ticks that the readout holds still while the
/// encoders are zeroed and LinuxCNC is asked to re-sync.
auto check_mode_change(LatheController& controller, Bench& bench, OperatingMode mode) -> void
{
    const auto before = controller.readout();
    controller.select_mode(mode);
    bool resynced = false;
    for (int step = 0; step < 5; ++step) {
        resynced = bench.tick().resync_motion || resynced;
        CHECK(controller.readout().x == Approx(before.x));
        CHECK(controller.readout().z == Approx(before.z));
    }
    CHECK(resynced);
    CHECK(bench.sample().position_x == Approx(0.0));
}

} // namespace

TEST_CASE("touch-off makes the current tool read the entered value", "[controller]")
{
    LatheController controller{settings_with_tools()};
    Bench bench{controller};
    controller.select_mode(OperatingMode::manual);
    bench.move_to({.x = 12.0, .z = -30.0});
    bench.settle();

    controller.set_axis(Axis::x, 5.0);
    controller.set_axis(Axis::z, 0.0);
    CHECK(controller.readout().x == Approx(5.0));
    CHECK(controller.readout().z == Approx(0.0));
}

TEST_CASE("changing modes zeroes the encoders without moving the readout", "[controller]")
{
    LatheController controller{settings_with_tools()};
    Bench bench{controller};
    controller.select_mode(OperatingMode::manual);
    bench.settle();
    bench.move_to({.x = 12.0, .z = -30.0});
    bench.settle();
    controller.set_axis(Axis::x, 5.0);
    controller.set_axis(Axis::z, -1.0);

    for (const OperatingMode mode :
         {OperatingMode::cycle, OperatingMode::manual, OperatingMode::other, OperatingMode::manual}) {
        bench.move_to({.x = bench.sample().position_x + 0.5, .z = bench.sample().position_z - 2.0});
        bench.settle();
        check_mode_change(controller, bench, mode);
    }
}

TEST_CASE("editing the current tool's offsets takes effect immediately", "[controller]")
{
    LatheController controller{settings_with_tools()};
    Bench bench{controller};
    bench.move_to({.x = 10.0, .z = 0.0});
    bench.settle();
    const double before = controller.readout().x;

    auto tools = controller.settings().tools;
    tools.at(1).offset_x += 1.0;
    controller.set_tools(tools);
    CHECK(controller.readout().x == Approx(before + 1.0));

    controller.select_tool(reference_tool);
    CHECK(controller.readout().x == Approx(before + 7.95));
}

TEST_CASE("cycles use the tool-tip position the operator sees", "[controller]")
{
    LatheController controller{settings_with_tools()};
    Bench bench{controller};
    bench.move_to({.x = 20.0, .z = 3.0});
    bench.settle();
    controller.set_axis(Axis::x, 6.0);
    CHECK(controller.work_position().x == Approx(controller.readout().x));
    CHECK(controller.work_position().x == Approx(6.0));
}

TEST_CASE("spindle angle wraps into 0..360 for either direction of rotation", "[controller]")
{
    LatheController controller{settings_with_tools()};
    Bench bench{controller};
    bench.sample().spindle_revs = -0.25;
    bench.settle();
    CHECK(controller.readout().a_degrees == Approx(270.0));

    controller.set_axis(Axis::a, 90.0);
    bench.sample().spindle_revs = -0.5;
    bench.settle();
    CHECK(controller.readout().a_degrees == Approx(0.0).margin(1e-9));
}

TEST_CASE("the controller starts idle: steppers unpowered, nothing follows the spindle",
          "[controller][leadscrew]")
{
    LatheController controller{settings_with_tools()};
    Bench bench{controller};
    CHECK(controller.direction() == DirectionMode::idle);
    bench.sample().spindle_rps = 5.0;
    bench.settle();
    const auto outputs = bench.tick().outputs;
    CHECK(!outputs.enable_stepper_z);
    CHECK(!outputs.enable_stepper_x);
    CHECK(!outputs.enable_z);
    CHECK(!outputs.enable_x);
    CHECK(outputs.forward_z == Approx(0.0).margin(1e-12));
    CHECK(outputs.forward_x == Approx(0.0).margin(1e-12));
}

TEST_CASE("entering canned cycles hands the axes to motion and leaves manual safe",
          "[controller][leadscrew]")
{
    LatheController controller{settings_with_tools()};
    Bench bench{controller};
    controller.select_mode(OperatingMode::manual);
    controller.set_direction(DirectionMode::forward);
    CHECK(bench.tick().outputs.enable_stepper_z);

    controller.select_mode(OperatingMode::cycle);
    const auto outputs = bench.tick().outputs;
    CHECK(outputs.control_source);
    CHECK(controller.feed() == FeedMode::back_compound);
    CHECK(controller.direction() == DirectionMode::hold);

    controller.select_mode(OperatingMode::manual);
    const auto manual = bench.tick().outputs;
    CHECK(!manual.control_source);
    CHECK(!manual.enable_stepper_z);
    CHECK(!manual.enable_stepper_x);
    CHECK(manual.enable_z);
    CHECK(manual.enable_x);
}

TEST_CASE("changing the gearing re-latches it to the current spindle and stepper positions",
          "[controller][leadscrew]")
{
    LatheController controller{settings_with_tools()};
    Bench bench{controller};
    controller.select_mode(OperatingMode::manual);
    bench.sample().spindle_revs = 41.5;
    bench.sample().stepper_z = -12.0;
    bench.settle();
    controller.set_pitches({.x = 0.02, .z = 1.5});
    const auto outputs = bench.tick().outputs;
    CHECK(outputs.offset_z_encoder == Approx(-41.5));
    CHECK(outputs.offset_z_stepper == Approx(-12.0));
    CHECK(outputs.forward_z == Approx(-1.5));
}

TEST_CASE("jogging drives velocity mode and stops when the panel goes quiet",
          "[controller][jog][safety]")
{
    LatheController controller{settings_with_tools()};
    Bench bench{controller};
    controller.select_mode(OperatingMode::manual);
    bench.settle();

    controller.jog_hold(JogButton::z_minus, bench.now());
    auto outputs = bench.tick().outputs;
    CHECK(outputs.control_z_type);
    CHECK(outputs.velocity_z_cmd < 0.0);

    // No refresh: the dead-man timeout stops the slide.
    for (int step = 0; step < 20; ++step) {
        outputs = bench.tick().outputs;
    }
    CHECK(!outputs.control_z_type);
    CHECK(outputs.velocity_z_cmd == Approx(0.0));
}

TEST_CASE("jogging is ignored outside manual mode", "[controller][jog][safety]")
{
    LatheController controller{settings_with_tools()};
    Bench bench{controller};
    controller.select_mode(OperatingMode::cycle);
    controller.jog_hold(JogButton::x_plus, bench.now());
    CHECK(!bench.tick().outputs.control_x_type);
}

TEST_CASE("encoder scales come from the settings on every tick", "[controller]")
{
    LatheController controller{settings_with_tools()};
    Bench bench{controller};
    controller.set_encoder_scales(0.0005, -0.002);
    const auto outputs = bench.tick().outputs;
    CHECK(outputs.scale_encoder_z == Approx(0.0005));
    CHECK(outputs.scale_encoder_x == Approx(-0.002));
}

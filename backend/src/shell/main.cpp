// elle: LinuxCNC DISPLAY program for CNC-converted lathes. Owns the `lathe` HAL component, runs the
// 100 Hz machine control loop, talks to LinuxCNC's task controller and shows the operator panel.

#include "panel_window.hpp"

#include <elle/app/control_loop.hpp>
#include <elle/app/ini.hpp>
#include <elle/app/options.hpp>
#include <elle/app/protocol.hpp>
#include <elle/app/session.hpp>
#include <elle/app/settings.hpp>
#include <elle/app/task_worker.hpp>
#include <elle/core/lathe_controller.hpp>
#include <elle/core/task.hpp>
#include <elle/linuxcnc/lathe_component.hpp>
#include <elle/linuxcnc/task_client.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <expected>
#include <filesystem>
#include <format>
#include <iterator>
#include <memory>
#include <optional>
#include <print>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

// glibc defines pid_t and the wait-status macros in private headers that include-cleaner cannot
// map back to the POSIX headers that declare them; these are the correct includes.
// NOLINTBEGIN(misc-include-cleaner)
#include <spawn.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
// NOLINTEND(misc-include-cleaner)

namespace {

using elle::app::Options;

constexpr std::string_view default_nml_file = "/usr/share/linuxcnc/linuxcnc.nml";

/// The HAL pins of the real machine.
class HalMachine final : public elle::app::MachineIo {
public:
    explicit HalMachine(elle::linuxcnc::LatheComponent component)
        : component_{std::move(component)}
    {
    }

    [[nodiscard]] auto read() -> elle::core::MachineSample override { return component_.read(); }
    auto write(const elle::core::PinOutputs& outputs) -> void override
    {
        component_.write(outputs);
    }

private:
    elle::linuxcnc::LatheComponent component_;
};

/// Runs a program and waits for it, without a shell.
// NOLINTBEGIN(misc-include-cleaner): pid_t and WIFEXITED/WEXITSTATUS, see the includes above.
auto run_program(std::vector<std::string> arguments) -> std::expected<void, std::string>
{
    std::vector<char*> argv{};
    argv.reserve(arguments.size() + 1);
    std::ranges::transform(arguments, std::back_inserter(argv),
                           [](std::string& argument) { return argument.data(); });
    argv.push_back(nullptr);
    pid_t child{};
    if (const int error =
            posix_spawnp(&child, argv.front(), nullptr, nullptr, argv.data(), environ);
        error != 0) {
        return std::unexpected{std::format("cannot start {}: {}", arguments.front(),
                                           std::generic_category().message(error))};
    }
    int status{};
    if (waitpid(child, &status, 0) < 0 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        return std::unexpected{std::format("{} failed", arguments.front())};
    }
    return {};
}

// NOLINTEND(misc-include-cleaner)

auto executable_directory() -> std::filesystem::path
{
    std::error_code error{};
    const auto self = std::filesystem::read_symlink("/proc/self/exe", error);
    return error ? std::filesystem::current_path() : self.parent_path();
}

/// Built panel: --ui, else the installed copy, else the source tree's build (development).
auto ui_directory(const Options& options) -> std::filesystem::path
{
    if (options.ui_directory.has_value()) {
        return *options.ui_directory;
    }
    const std::filesystem::path installed = executable_directory() / "../share/elle/ui";
    if (std::filesystem::exists(installed / "index.html")) {
        return std::filesystem::weakly_canonical(installed);
    }
    return ELLE_SOURCE_UI_DIRECTORY;
}

/// The machine side: HAL component, post-GUI wiring and NML connection, or the simulation.
struct Machine {
    std::unique_ptr<elle::app::MachineIo> io{};
    std::unique_ptr<elle::core::TaskPort> task{};
    std::filesystem::path cycle_directory{};
};

auto connect_machine(const Options& options, std::vector<std::string>& log)
    -> std::expected<Machine, std::string>
{
    if (options.demo) {
        log.emplace_back("Demo mode: simulated machine, LinuxCNC is not used.");
        return Machine{.io = std::make_unique<elle::app::SimulatedMachine>(),
                       .task = std::make_unique<elle::app::DemoTask>(),
                       .cycle_directory = std::filesystem::temp_directory_path()};
    }
    if (!options.ini.has_value()) {
        return std::unexpected{std::string{"no INI file given"}};
    }
    const std::filesystem::path ini_file = std::filesystem::absolute(*options.ini);
    const auto ini = elle::app::IniFile::load(ini_file);
    if (!ini.has_value()) {
        return std::unexpected{ini.error()};
    }
    const std::filesystem::path directory = ini_file.parent_path();

    auto component = elle::linuxcnc::LatheComponent::create();
    if (!component.has_value()) {
        return std::unexpected{component.error()};
    }
    if (const auto ready = component->ready(); !ready.has_value()) {
        return std::unexpected{ready.error()};
    }
    log.emplace_back("HAL component 'lathe' ready.");

    if (const auto postgui = ini->find("HAL", "POSTGUI_HALFILE"); postgui.has_value()) {
        const auto wired = run_program(
            {"halcmd", "-i", ini_file.string(), "-f", (directory / *postgui).string()});
        if (!wired.has_value()) {
            return std::unexpected{std::format("POSTGUI_HALFILE: {}", wired.error())};
        }
        log.push_back(std::format("Wired {}.", *postgui));
    }

    const std::filesystem::path nml_file =
        ini->find("EMC", "NML_FILE").value_or(std::string{default_nml_file});
    auto task = elle::linuxcnc::TaskClient::connect(nml_file);
    if (!task.has_value()) {
        return std::unexpected{task.error()};
    }
    log.emplace_back("Connected to LinuxCNC.");
    return Machine{.io = std::make_unique<HalMachine>(std::move(*component)),
                   .task = std::make_unique<elle::linuxcnc::TaskClient>(std::move(*task)),
                   .cycle_directory = directory};
}

auto run(const Options& options) -> int
{
    std::vector<std::string> log{};
    const auto settings_file = elle::app::settings_path();
    auto loaded = elle::app::load_settings(settings_file, elle::app::legacy_settings_path());
    log.push_back(std::format("Settings: {}.", loaded.note));

    auto machine = connect_machine(options, log);
    if (!machine.has_value()) {
        std::println(stderr, "elle: {}", machine.error());
        return EXIT_FAILURE;
    }
    if (!elle::shell::initialize_gtk()) {
        std::println(stderr, "elle: cannot open a display");
        return EXIT_FAILURE;
    }

    elle::app::Session session{std::move(loaded.settings), settings_file};
    elle::app::TaskWorker worker{*machine->task, machine->cycle_directory};
    const elle::app::ControlLoop loop{session, *machine->io, worker};

    std::unique_ptr<elle::shell::PanelWindow> window{};
    const auto on_message = [&](std::string_view json) {
        auto outcome = session.handle(json, elle::app::Session::Clock::now());
        for (const auto& reply : outcome.replies) {
            window->post(reply);
        }
        for (auto& job : outcome.jobs) {
            worker.submit(std::move(job));
        }
        if (outcome.quit) {
            window->quit();
        }
    };
    const auto on_refresh = [&] {
        session.observe(worker.snapshot());
        window->post(session.status());
        auto lines = worker.take_log();
        lines.insert(lines.begin(), log.begin(), log.end());
        log.clear();
        if (!lines.empty()) {
            window->post(elle::app::log_message(lines));
        }
    };
    window = std::make_unique<elle::shell::PanelWindow>(
        elle::shell::PanelOptions{.ui_directory = ui_directory(options),
                                  .dev_url = options.dev_url,
                                  .windowed = options.windowed},
        elle::shell::PanelCallbacks{.message = on_message, .refresh = on_refresh});
    window->run();
    return EXIT_SUCCESS;
}

} // namespace

auto main(int argc, char** argv) -> int
{
    try {
        const std::span<char*> raw{argv, static_cast<std::size_t>(argc)};
        const std::vector<std::string_view> arguments{std::from_range, raw.subspan(1)};
        const auto options = elle::app::parse_options(arguments);
        if (!options.has_value()) {
            std::println(stderr, "elle: {}\n\n{}", options.error(), elle::app::usage());
            return EXIT_FAILURE;
        }
        if (options->help) {
            std::print("{}", elle::app::usage());
            return EXIT_SUCCESS;
        }
        return run(*options);
    } catch (const std::exception& error) {
        static_cast<void>(std::fputs("elle: ", stderr));
        static_cast<void>(std::fputs(error.what(), stderr));
        static_cast<void>(std::fputs("\n", stderr));
        return EXIT_FAILURE;
    } catch (...) {
        static_cast<void>(std::fputs("elle: unexpected error\n", stderr));
        return EXIT_FAILURE;
    }
}

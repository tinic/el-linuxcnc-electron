#pragma once

#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace elle::shell {

struct PanelOptions {
    /// Built panel files, served to the web view as elle://panel/.
    std::filesystem::path ui_directory{};
    /// Development server URL to load instead of the built files.
    std::optional<std::string> dev_url{};
    bool windowed{};
};

struct PanelCallbacks {
    /// A message from the panel (JSON text).
    std::function<void(std::string_view json)> message;
    /// Called every `refresh` on the UI thread, to push status to the panel.
    std::function<void()> refresh;
    /// SIGINT, SIGTERM or SIGHUP arrived (LinuxCNC shutting down, Ctrl+C): shut down on the UI
    /// thread exactly as if the operator had pressed Exit.
    std::function<void()> terminate;
};

/// The operator panel: a full-screen WebKitGTK window. The page talks to C++ in-process through
/// the `elle` script message handler and `window.elleReceive`; no network port is opened.
class PanelWindow {
public:
    static constexpr auto refresh_interval = std::chrono::milliseconds{33};

    PanelWindow(PanelOptions options, PanelCallbacks callbacks);
    PanelWindow(const PanelWindow&) = delete;
    PanelWindow(PanelWindow&&) = delete;
    auto operator=(const PanelWindow&) -> PanelWindow& = delete;
    auto operator=(PanelWindow&&) -> PanelWindow& = delete;
    ~PanelWindow();

    /// Delivers a message (JSON text) to the panel. UI thread only.
    auto post(std::string_view json) -> void;
    /// Runs the GTK main loop until `quit` or the window is closed.
    auto run() -> void;
    auto quit() -> void;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_{};
};

/// Initializes GTK; must be called once on the main thread before any window is created.
[[nodiscard]] auto initialize_gtk() -> bool;

} // namespace elle::shell

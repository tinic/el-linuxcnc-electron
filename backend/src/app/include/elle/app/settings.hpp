#pragma once

#include <elle/core/lathe_controller.hpp>

#include <array>
#include <expected>
#include <filesystem>
#include <string>

namespace elle::app {

/// Everything Elle persists between runs.
struct Settings {
    bool diameter_mode{true};
    bool metric_on_startup{true};
    int thread_preset_tab{};
    int turning_preset_tab{};
    std::array<int, 2> pitch_preset_tabs{}; ///< X, Z
    core::MachineSettings machine{};
};

/// The settings a fresh installation starts with: ten empty tool slots, T00 the reference tool.
[[nodiscard]] auto default_settings() -> Settings;

/// Where settings live: `$XDG_CONFIG_HOME/elle/settings.json`, else `~/.config/elle/settings.json`.
[[nodiscard]] auto settings_path() -> std::filesystem::path;

/// The Electron app's settings file, imported once if no Elle settings exist yet.
[[nodiscard]] auto legacy_settings_path() -> std::filesystem::path;

struct LoadedSettings {
    Settings settings{};
    std::string note{}; ///< what happened, for the log: loaded, imported, defaults and why
};

/// Reads `file`. If it does not exist, imports `legacy_file` instead; if neither can be read,
/// returns the defaults. A file that exists but cannot be parsed is kept as `<file>.broken`.
[[nodiscard]] auto load_settings(const std::filesystem::path& file,
                                 const std::filesystem::path& legacy_file) -> LoadedSettings;

/// Writes `file` atomically (temporary file, then rename).
[[nodiscard]] auto save_settings(const Settings& settings, const std::filesystem::path& file)
    -> std::expected<void, std::string>;

[[nodiscard]] auto to_json(const Settings& settings) -> std::string;

} // namespace elle::app

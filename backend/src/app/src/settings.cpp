#include <elle/app/settings.hpp>

#include <elle/core/lathe_controller.hpp>

#include <glaze/glaze.hpp>

#include <cstdlib>
#include <expected>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace elle::app {

namespace {

constexpr int default_tool_count = 10;

/// A tool in the Electron app's settings (camelCase keys).
struct LegacyTool {
    int id{};
    double offset_x{};
    double offset_z{};
    std::string description{};
};

/// The Electron app's `appConfig.json` "setting" object (camelCase keys).
struct LegacySetting {
    std::optional<bool> diameter_mode{};
    std::optional<bool> metric_on_startup{};
    std::optional<int> threading_tab{};
    std::optional<int> turning_tab{};
    std::optional<std::array<int, 2>> pitch_tabs{};
    std::optional<double> pitch_x{};
    std::optional<double> pitch_z{};
    std::optional<double> encoder_scale_z{};
    std::optional<double> encoder_scale_x{};
    std::optional<std::vector<LegacyTool>> tools{};
    std::optional<int> current_tool{};
};

struct LegacyConfig {
    LegacySetting setting{};
};

struct LenientJson : glz::opts {
    bool error_on_unknown_keys = false;
};

auto read_file(const std::filesystem::path& file) -> std::optional<std::string>
{
    std::ifstream stream{file, std::ios::binary};
    if (!stream.is_open()) {
        return std::nullopt;
    }
    return std::string{std::istreambuf_iterator<char>{stream}, std::istreambuf_iterator<char>{}};
}

auto config_home() -> std::filesystem::path
{
    // NOLINTNEXTLINE(concurrency-mt-unsafe): read once at start-up, before any thread exists.
    if (const char* xdg = std::getenv("XDG_CONFIG_HOME"); xdg != nullptr && *xdg != '\0') {
        return xdg;
    }
    // NOLINTNEXTLINE(concurrency-mt-unsafe): as above.
    if (const char* home = std::getenv("HOME"); home != nullptr && *home != '\0') {
        return std::filesystem::path{home} / ".config";
    }
    return ".config";
}

auto import_legacy(const LegacySetting& legacy) -> Settings
{
    Settings settings = default_settings();
    settings.diameter_mode = legacy.diameter_mode.value_or(settings.diameter_mode);
    settings.metric_on_startup = legacy.metric_on_startup.value_or(settings.metric_on_startup);
    settings.thread_preset_tab = legacy.threading_tab.value_or(0);
    settings.turning_preset_tab = legacy.turning_tab.value_or(0);
    settings.pitch_preset_tabs = legacy.pitch_tabs.value_or(std::array<int, 2>{});
    auto& machine = settings.machine;
    machine.pitches.x = legacy.pitch_x.value_or(machine.pitches.x);
    machine.pitches.z = legacy.pitch_z.value_or(machine.pitches.z);
    machine.encoder_scale_z = legacy.encoder_scale_z.value_or(machine.encoder_scale_z);
    machine.encoder_scale_x = legacy.encoder_scale_x.value_or(machine.encoder_scale_x);
    if (legacy.tools.has_value() && !legacy.tools->empty()) {
        machine.tools.clear();
        for (const LegacyTool& tool : *legacy.tools) {
            machine.tools.push_back({.id = tool.id,
                                     .offset_x = tool.offset_x,
                                     .offset_z = tool.offset_z,
                                     .description = tool.description});
        }
    }
    machine.current_tool = legacy.current_tool.value_or(0);
    return settings;
}

} // namespace

} // namespace elle::app

template <>
struct glz::meta<elle::app::LegacySetting> {
    using T = elle::app::LegacySetting;
    static constexpr auto value =
        glz::object("diameterMode", &T::diameter_mode, "defaultMetricOnStartup",
                    &T::metric_on_startup, "selectedThreadingTab", &T::threading_tab,
                    "selectedTurningTab", &T::turning_tab, "selectedPitchTab", &T::pitch_tabs,
                    "pitchX", &T::pitch_x, "pitchZ", &T::pitch_z, "encoderScaleZ",
                    &T::encoder_scale_z, "encoderScaleX", &T::encoder_scale_x, "tools", &T::tools,
                    "currentToolIndex", &T::current_tool);
};

template <>
struct glz::meta<elle::app::LegacyTool> {
    using T = elle::app::LegacyTool;
    static constexpr auto value = glz::object("id", &T::id, "offsetX", &T::offset_x, "offsetZ",
                                              &T::offset_z, "description", &T::description);
};

namespace elle::app {

auto default_settings() -> Settings
{
    Settings settings{};
    for (int id = 0; id < default_tool_count; ++id) {
        settings.machine.tools.push_back(
            {.id = id, .description = id == 0 ? "Reference Tool" : std::string{}});
    }
    return settings;
}

auto settings_path() -> std::filesystem::path
{
    return config_home() / "elle" / "settings.json";
}

auto legacy_settings_path() -> std::filesystem::path
{
    return config_home() / "elle-app" / "appConfig.json";
}

auto load_settings(const std::filesystem::path& file, const std::filesystem::path& legacy_file)
    -> LoadedSettings
{
    if (const auto text = read_file(file); text.has_value()) {
        Settings settings = default_settings();
        if (const auto error = glz::read<LenientJson{}>(settings, *text); !error) {
            return {.settings = std::move(settings), .note = std::format("loaded {}", file.string())};
        }
        std::error_code ignored;
        std::filesystem::rename(file, std::filesystem::path{file} += ".broken", ignored);
        return {.settings = default_settings(),
                .note = std::format("{} could not be parsed; kept as .broken, using defaults",
                                    file.string())};
    }
    if (const auto text = read_file(legacy_file); text.has_value()) {
        LegacyConfig legacy;
        if (const auto error = glz::read<LenientJson{}>(legacy, *text); !error) {
            return {.settings = import_legacy(legacy.setting),
                    .note = std::format("imported {}", legacy_file.string())};
        }
    }
    return {.settings = default_settings(), .note = "no settings found, using defaults"};
}

auto to_json(const Settings& settings) -> std::string
{
    std::string json{};
    if (glz::write<glz::opts{.prettify = true}>(settings, json)) {
        json.clear();
    }
    return json;
}

auto save_settings(const Settings& settings, const std::filesystem::path& file)
    -> std::expected<void, std::string>
{
    std::error_code error{};
    std::filesystem::create_directories(file.parent_path(), error);
    if (error) {
        return std::unexpected{std::format("creating {}: {}", file.parent_path().string(),
                                           error.message())};
    }
    const std::filesystem::path temporary = std::filesystem::path{file} += ".tmp";
    {
        std::ofstream stream{temporary, std::ios::binary | std::ios::trunc};
        stream << to_json(settings);
        if (!stream.flush()) {
            return std::unexpected{std::format("writing {} failed", temporary.string())};
        }
    }
    std::filesystem::rename(temporary, file, error);
    if (error) {
        return std::unexpected{std::format("replacing {}: {}", file.string(), error.message())};
    }
    return {};
}

} // namespace elle::app

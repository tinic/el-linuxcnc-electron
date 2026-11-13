#include <elle/app/settings.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <system_error>

using Catch::Approx;

namespace {

/// A fresh, empty directory under the system temporary directory, removed afterwards.
class ScratchDirectory {
public:
    explicit ScratchDirectory(std::string_view name)
        : path_{std::filesystem::temp_directory_path() / name}
    {
        std::filesystem::remove_all(path_);
        std::filesystem::create_directories(path_);
    }
    ScratchDirectory(const ScratchDirectory&) = delete;
    ScratchDirectory(ScratchDirectory&&) = delete;
    auto operator=(const ScratchDirectory&) -> ScratchDirectory& = delete;
    auto operator=(ScratchDirectory&&) -> ScratchDirectory& = delete;
    ~ScratchDirectory()
    {
        std::error_code ignored;
        std::filesystem::remove_all(path_, ignored);
    }

    [[nodiscard]] auto path() const -> const std::filesystem::path& { return path_; }

private:
    std::filesystem::path path_{};
};

auto write_text(const std::filesystem::path& file, std::string_view text) -> void
{
    std::filesystem::create_directories(file.parent_path());
    std::ofstream{file} << text;
}

constexpr std::string_view legacy_config = R"({
  "setting": {
    "appBounds": {"x": 0, "y": 0, "width": 1366, "height": 768},
    "diameterMode": true,
    "defaultMetricOnStartup": false,
    "selectedThreadingTab": 2,
    "selectedTurningTab": 1,
    "selectedPitchTab": [1, 0],
    "pitchX": 0.02,
    "pitchZ": 0.05,
    "tools": [
      {"id": 0, "offsetX": 0, "offsetZ": 0, "description": "Reference Tool"},
      {"id": 1, "offsetX": -7.95, "offsetZ": 0, "description": "Cutoff Tool"}
    ],
    "currentToolIndex": 1,
    "currentToolOffsetX": -7.95,
    "currentToolOffsetZ": 0,
    "encoderScaleZ": 0.001,
    "encoderScaleX": -0.001
  }
})";

} // namespace

TEST_CASE("the Electron app's settings are imported once", "[settings]")
{
    const ScratchDirectory scratch{"elle-settings-import"};
    const auto legacy = scratch.path() / "elle-app" / "appConfig.json";
    const auto current = scratch.path() / "elle" / "settings.json";
    write_text(legacy, legacy_config);

    const auto loaded = elle::app::load_settings(current, legacy);
    CHECK(loaded.note.starts_with("imported"));
    const auto& settings = loaded.settings;
    CHECK(settings.diameter_mode);
    CHECK(!settings.metric_on_startup);
    CHECK(settings.thread_preset_tab == 2);
    CHECK(settings.pitch_preset_tabs.at(0) == 1);
    CHECK(settings.machine.pitches.z == Approx(0.05));
    REQUIRE(settings.machine.tools.size() == 2);
    CHECK(settings.machine.tools.at(1).offset_x == Approx(-7.95));
    CHECK(settings.machine.tools.at(1).description == "Cutoff Tool");
    CHECK(settings.machine.current_tool == 1);
}

TEST_CASE("settings survive a save and load", "[settings]")
{
    const ScratchDirectory scratch{"elle-settings-roundtrip"};
    const auto file = scratch.path() / "elle" / "settings.json";

    auto settings = elle::app::default_settings();
    settings.machine.tools.at(3).offset_z = 1.25;
    settings.machine.tools.at(3).description = "Threading";
    settings.machine.encoder_scale_x = -0.0005;
    REQUIRE(elle::app::save_settings(settings, file).has_value());

    const auto loaded = elle::app::load_settings(file, scratch.path() / "missing.json");
    CHECK(loaded.note.starts_with("loaded"));
    CHECK(elle::app::to_json(loaded.settings) == elle::app::to_json(settings));
}

TEST_CASE("a damaged settings file is kept aside and defaults are used", "[settings]")
{
    const ScratchDirectory scratch{"elle-settings-broken"};
    const auto file = scratch.path() / "settings.json";
    write_text(file, "{ not json");

    const auto loaded = elle::app::load_settings(file, scratch.path() / "missing.json");
    CHECK(elle::app::to_json(loaded.settings) == elle::app::to_json(elle::app::default_settings()));
    CHECK(std::filesystem::exists(scratch.path() / "settings.json.broken"));
}

TEST_CASE("settings saved before the tool post setting load as a front tool post", "[settings]")
{
    const ScratchDirectory scratch{"elle-settings-tool-post"};
    const auto file = scratch.path() / "elle" / "settings.json";
    auto settings = elle::app::default_settings();
    REQUIRE(elle::app::save_settings(settings, file).has_value());
    std::ifstream saved{file};
    std::string text{std::istreambuf_iterator<char>{saved}, std::istreambuf_iterator<char>{}};
    saved.close();
    const auto key = text.find("\"back_tool_post\"");
    REQUIRE(key != std::string::npos);
    const auto comma = text.find(',', key);
    text.erase(key, comma - key + 1); // an older file without the key
    write_text(file, text);

    const auto loaded = elle::app::load_settings(file, scratch.path() / "missing.json");
    CHECK(loaded.note.starts_with("loaded"));
    CHECK(!loaded.settings.back_tool_post);
}

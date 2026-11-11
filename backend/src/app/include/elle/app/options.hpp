#pragma once

#include <expected>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace elle::app {

struct Options {
    /// LinuxCNC's INI file. LinuxCNC passes `-ini <file>` to its DISPLAY program.
    std::optional<std::filesystem::path> ini{};
    /// Load the panel from a development server (`npm run dev`) instead of the built files.
    std::optional<std::string> dev_url{};
    /// Directory with the built panel (`ui/dist`); defaults to the installed location.
    std::optional<std::filesystem::path> ui_directory{};
    /// A normal window instead of full-screen kiosk mode.
    bool windowed{};
    /// No HAL and no LinuxCNC: a simulated machine, for trying the panel anywhere.
    bool demo{};
    bool help{};
};

[[nodiscard]] auto parse_options(std::span<const std::string_view> arguments)
    -> std::expected<Options, std::string>;

[[nodiscard]] auto usage() -> std::string_view;

} // namespace elle::app

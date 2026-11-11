#include <elle/app/options.hpp>

#include <cstddef>
#include <expected>
#include <format>
#include <span>
#include <string>
#include <string_view>

namespace elle::app {

auto usage() -> std::string_view
{
    return "usage: elle -ini <lathe.ini> [--windowed] [--dev-url <url>] [--ui <dir>]\n"
           "       elle --demo [--windowed] [--dev-url <url>] [--ui <dir>]\n"
           "\n"
           "Operator panel and machine controller for Elle. LinuxCNC starts it as its DISPLAY\n"
           "program with -ini; --demo runs it with a simulated machine instead.\n";
}

auto parse_options(std::span<const std::string_view> arguments) -> std::expected<Options, std::string>
{
    Options options{};
    for (std::size_t index = 0; index < arguments.size(); ++index) {
        const std::string_view argument = arguments[index];
        const auto value = [&]() -> std::expected<std::string_view, std::string> {
            if (index + 1 >= arguments.size()) {
                return std::unexpected{std::format("{} needs a value", argument)};
            }
            ++index;
            return arguments[index];
        };
        if (argument == "-ini" || argument == "--ini") {
            const auto file = value();
            if (!file.has_value()) {
                return std::unexpected{file.error()};
            }
            options.ini = std::filesystem::path{*file};
        } else if (argument == "--dev-url") {
            const auto url = value();
            if (!url.has_value()) {
                return std::unexpected{url.error()};
            }
            options.dev_url = std::string{*url};
        } else if (argument == "--ui") {
            const auto directory = value();
            if (!directory.has_value()) {
                return std::unexpected{directory.error()};
            }
            options.ui_directory = std::filesystem::path{*directory};
        } else if (argument == "--windowed") {
            options.windowed = true;
        } else if (argument == "--demo") {
            options.demo = true;
        } else if (argument == "--help" || argument == "-h") {
            options.help = true;
        } else if (argument == "--") {
            continue; // LinuxCNC separates its own arguments with "--"
        } else {
            return std::unexpected{std::format("unknown argument '{}'", argument)};
        }
    }
    if (!options.help && !options.demo && !options.ini.has_value()) {
        return std::unexpected{std::string{"either -ini <file> or --demo is required"}};
    }
    return options;
}

} // namespace elle::app

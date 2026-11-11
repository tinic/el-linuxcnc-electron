#include <elle/app/options.hpp>

#include <cstddef>
#include <expected>
#include <format>
#include <span>
#include <string>
#include <string_view>

namespace elle::app {

namespace {

auto takes_value(std::string_view argument) -> bool
{
    return argument == "-ini" || argument == "--ini" || argument == "--dev-url" ||
           argument == "--ui";
}

/// An option that takes a value, e.g. `--ui dist`.
struct Setting {
    std::string_view name{};
    std::string_view value{};
};

auto set_value(Options& options, const Setting& setting) -> void
{
    if (setting.name == "--dev-url") {
        options.dev_url = std::string{setting.value};
    } else if (setting.name == "--ui") {
        options.ui_directory = std::filesystem::path{setting.value};
    } else {
        options.ini = std::filesystem::path{setting.value};
    }
}

auto set_flag(Options& options, std::string_view argument) -> bool
{
    if (argument == "--windowed") {
        options.windowed = true;
    } else if (argument == "--demo") {
        options.demo = true;
    } else if (argument == "--help" || argument == "-h") {
        options.help = true;
    } else if (argument != "--") { // LinuxCNC separates its own arguments with "--"
        return false;
    }
    return true;
}

} // namespace

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
        if (takes_value(argument)) {
            if (index + 1 >= arguments.size()) {
                return std::unexpected{std::format("{} needs a value", argument)};
            }
            ++index;
            set_value(options, {.name = argument, .value = arguments[index]});
        } else if (!set_flag(options, argument)) {
            return std::unexpected{std::format("unknown argument '{}'", argument)};
        }
    }
    if (!options.help && !options.demo && !options.ini.has_value()) {
        return std::unexpected{std::string{"either -ini <file> or --demo is required"}};
    }
    return options;
}

} // namespace elle::app

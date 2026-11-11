#include <elle/app/ini.hpp>

#include <algorithm>
#include <expected>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>

namespace elle::app {

namespace {

constexpr std::string_view whitespace = " \t\r";

auto trim(std::string_view text) -> std::string_view
{
    const auto first = text.find_first_not_of(whitespace);
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = text.find_last_not_of(whitespace);
    return text.substr(first, last - first + 1);
}

} // namespace

auto IniFile::parse(std::string_view text) -> IniFile
{
    IniFile ini{};
    std::string section{};
    while (!text.empty()) {
        const auto end = std::min(text.find('\n'), text.size());
        const std::string_view line = trim(text.substr(0, end));
        text.remove_prefix(std::min(end + 1, text.size()));

        if (line.empty() || line.front() == '#' || line.front() == ';') {
            continue;
        }
        if (line.front() == '[' && line.back() == ']') {
            section = trim(line.substr(1, line.size() - 2));
            continue;
        }
        const auto equals = line.find('=');
        if (equals == std::string_view::npos) {
            continue;
        }
        ini.entries_.push_back({.section = section,
                                .key = std::string{trim(line.substr(0, equals))},
                                .value = std::string{trim(line.substr(equals + 1))}});
    }
    return ini;
}

auto IniFile::load(const std::filesystem::path& file) -> std::expected<IniFile, std::string>
{
    std::ifstream stream{file};
    if (!stream.is_open()) {
        return std::unexpected{std::format("cannot read {}", file.string())};
    }
    const std::string text{std::istreambuf_iterator<char>{stream}, std::istreambuf_iterator<char>{}};
    return parse(text);
}

auto IniFile::find(std::string_view section, std::string_view key) const
    -> std::optional<std::string>
{
    const auto entry = std::ranges::find_if(entries_, [&](const Entry& candidate) {
        return candidate.section == section && candidate.key == key;
    });
    if (entry == entries_.end()) {
        return std::nullopt;
    }
    return entry->value;
}

} // namespace elle::app

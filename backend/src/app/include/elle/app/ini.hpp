#pragma once

#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace elle::app {

/// A LinuxCNC INI file: `[SECTION]` headers, `KEY = VALUE` lines, `#` and `;` comments.
class IniFile {
public:
    [[nodiscard]] static auto parse(std::string_view text) -> IniFile;
    [[nodiscard]] static auto load(const std::filesystem::path& file)
        -> std::expected<IniFile, std::string>;

    /// The first value of `key` in `section`, as LinuxCNC's own lookup returns it.
    [[nodiscard]] auto find(std::string_view section, std::string_view key) const
        -> std::optional<std::string>;

private:
    struct Entry {
        std::string section;
        std::string key;
        std::string value;
    };
    std::vector<Entry> entries_{};
};

} // namespace elle::app

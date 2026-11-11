#include <elle/app/ini.hpp>

#include <catch2/catch_test_macros.hpp>

#include <optional>
#include <string>

TEST_CASE("INI values are found by section and key", "[ini]")
{
    const auto ini = elle::app::IniFile::parse(R"(
# comment
[DISPLAY]
DISPLAY = elle --windowed
GEOMETRY=XZ

[HAL]
HALFILE = lathe.hal
  POSTGUI_HALFILE   =   postgui.hal  ; not a comment for LinuxCNC either
[RS274NGC]
RS274NGC_STARTUP_CODE = G18 G8 G21
)");
    CHECK(ini.find("DISPLAY", "DISPLAY") == std::optional<std::string>{"elle --windowed"});
    CHECK(ini.find("DISPLAY", "GEOMETRY") == std::optional<std::string>{"XZ"});
    CHECK(ini.find("HAL", "POSTGUI_HALFILE") ==
          std::optional<std::string>{"postgui.hal  ; not a comment for LinuxCNC either"});
    CHECK(ini.find("RS274NGC", "RS274NGC_STARTUP_CODE") ==
          std::optional<std::string>{"G18 G8 G21"});
    CHECK(!ini.find("HAL", "DISPLAY").has_value());
    CHECK(!ini.find("EMC", "NML_FILE").has_value());
}

TEST_CASE("the first of repeated keys wins", "[ini]")
{
    const auto ini = elle::app::IniFile::parse("[HAL]\nHALFILE = a.hal\nHALFILE = b.hal\n");
    CHECK(ini.find("HAL", "HALFILE") == std::optional<std::string>{"a.hal"});
}

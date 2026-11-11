#include <elle/app/options.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <string_view>

using elle::app::parse_options;

TEST_CASE("LinuxCNC's DISPLAY invocation is understood", "[options]")
{
    constexpr std::array<std::string_view, 2> linuxcnc{"-ini", "/home/turo/machine/lathe.ini"};
    const auto options = parse_options(linuxcnc);
    REQUIRE(options.has_value());
    CHECK(options->ini == "/home/turo/machine/lathe.ini");
    CHECK(!options->demo);
    CHECK(!options->windowed);
}

TEST_CASE("extra DISPLAY arguments come before the INI file", "[options]")
{
    constexpr std::array<std::string_view, 5> args{"--windowed", "--dev-url",
                                                   "http://127.0.0.1:5173/", "-ini", "lathe.ini"};
    const auto options = parse_options(args);
    REQUIRE(options.has_value());
    CHECK(options->windowed);
    CHECK(options->dev_url == "http://127.0.0.1:5173/");
}

TEST_CASE("demo mode needs no INI file", "[options]")
{
    constexpr std::array<std::string_view, 1> args{"--demo"};
    const auto options = parse_options(args);
    REQUIRE(options.has_value());
    CHECK(options->demo);
}

TEST_CASE("bad command lines are rejected", "[options]")
{
    CHECK(!parse_options({}).has_value());
    constexpr std::array<std::string_view, 1> dangling{"-ini"};
    CHECK(!parse_options(dangling).has_value());
    constexpr std::array<std::string_view, 2> unknown{"--demo", "--turbo"};
    CHECK(!parse_options(unknown).has_value());
}

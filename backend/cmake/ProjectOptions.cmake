# Project-wide build policy.
#
# Two layers:
#  * Directory-scoped flags (add_compile_options / add_link_options) for anything that changes ABI or
#    instrumentation: sanitizers, libstdc++ debug mode, coverage, hardening. These must also reach the
#    third-party code we build from source (Catch2), or objects disagree about layouts.
#  * The elle::project_options interface target for language level and warnings. Only our own targets
#    link it; third-party code is pulled in as SYSTEM so its headers never trip our warnings.

include_guard(GLOBAL)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_VERSION VERSION_LESS 16)
  message(FATAL_ERROR "Elle is built with GCC 16 or newer (found ${CMAKE_CXX_COMPILER_ID} "
                      "${CMAKE_CXX_COMPILER_VERSION}). Use one of the presets in CMakePresets.json.")
endif()

option(ELLE_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)
option(ELLE_ENABLE_HARDENING "Enable -fhardened and related run-time hardening" ON)
option(ELLE_ENABLE_LIBSTDCXX_DEBUG "Use the checked libstdc++ (_GLIBCXX_DEBUG); changes ABI" OFF)
option(ELLE_ENABLE_COVERAGE "Instrument for gcov coverage" OFF)
option(ELLE_ENABLE_GCC_ANALYZER "Run GCC's static analyzer (-fanalyzer) during compilation" OFF)
set(ELLE_SANITIZERS "" CACHE STRING "Semicolon-separated sanitizers, e.g. address;undefined or thread")

# Binaries built by GCC 16 need its libstdc++ at run time; Debian 12 ships an older one.
get_filename_component(_elle_gcc_bin "${CMAKE_CXX_COMPILER}" DIRECTORY)
get_filename_component(_elle_gcc_prefix "${_elle_gcc_bin}" DIRECTORY)
if(EXISTS "${_elle_gcc_prefix}/lib64/libstdc++.so")
  set(CMAKE_BUILD_RPATH "${_elle_gcc_prefix}/lib64")
endif()

# Bit-for-bit reproducible floating point: the cycle generators must match the reference
# implementation exactly, so never contract a*b+c into a fused multiply-add.
add_compile_options(-ffp-contract=off)

if(ELLE_ENABLE_HARDENING)
  # -fhardened (GCC 14+) enables _FORTIFY_SOURCE=3, _GLIBCXX_ASSERTIONS, -ftrivial-auto-var-init=zero,
  # PIE, -fstack-protector-strong, -fstack-clash-protection, -fcf-protection=full and full RELRO.
  # _FORTIFY_SOURCE needs optimization, so -fhardened is skipped in unoptimized builds.
  add_compile_options($<$<NOT:$<CONFIG:Debug>>:-fhardened> $<$<CONFIG:Debug>:-D_GLIBCXX_ASSERTIONS>
                      $<$<CONFIG:Debug>:-fstack-protector-strong>)
  # -fhardened drops its own linker part whenever any link option is given, so the linker
  # hardening is spelled out instead: PIE, full RELRO, no executable stack.
  add_link_options(-pie LINKER:-z,relro LINKER:-z,now LINKER:-z,noexecstack)
endif()

if(ELLE_ENABLE_LIBSTDCXX_DEBUG)
  add_compile_definitions(_GLIBCXX_DEBUG _GLIBCXX_DEBUG_PEDANTIC _GLIBCXX_DEBUG_BACKTRACE)
  link_libraries(stdc++exp) # after the objects, so its symbols resolve
endif()

if(ELLE_SANITIZERS)
  list(JOIN ELLE_SANITIZERS "," _elle_sanitizers)
  add_compile_options(-fsanitize=${_elle_sanitizers} -fno-sanitize-recover=all
                      -fno-omit-frame-pointer)
  add_link_options(-fsanitize=${_elle_sanitizers})
endif()

if(ELLE_ENABLE_COVERAGE)
  add_compile_options(--coverage -O0 -fno-inline -fprofile-update=atomic)
  add_link_options(--coverage)
endif()

add_library(elle_project_options INTERFACE)
add_library(elle::project_options ALIAS elle_project_options)
target_compile_features(elle_project_options INTERFACE cxx_std_23)

# The baseline from cppbestpractices "Use the Tools Available", extended with every further GCC
# diagnostic that is meaningful for this code base. Warnings deliberately NOT enabled:
#   -Wpadded           reports layout, not defects; there is no fix for most hits.
#   -Waggregate-return C-era rule against returning structs; contradicts value semantics.
#   -Wsuggest-attribute=pure|const  advisory, fires on most small functions across TUs.
#   -Winline           reports optimizer decisions, not code problems.
#   -Wsystem-headers   third-party code is outside our control by design.
set(ELLE_GCC_WARNINGS
    # cppbestpractices list (GCC-applicable entries)
    -Wall
    -Wextra
    -Wpedantic
    -pedantic-errors
    -Wshadow
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Wcast-align=strict
    -Wunused
    -Woverloaded-virtual=2
    -Wconversion
    -Wsign-conversion
    -Wmisleading-indentation
    -Wduplicated-cond
    -Wduplicated-branches
    -Wlogical-op
    -Wnull-dereference
    -Wuseless-cast
    -Wdouble-promotion
    -Wformat=2
    -Wimplicit-fallthrough=5
    -Weffc++
    # further GCC diagnostics
    -Walloca
    -Warith-conversion
    -Warray-bounds=2
    -Wcast-qual
    -Wcatch-value=3
    -Wcomma-subscript
    -Wconditionally-supported
    -Wctad-maybe-unsupported
    -Wctor-dtor-privacy
    -Wdangling-reference
    -Wdate-time
    -Wdeprecated-copy-dtor
    -Wextra-semi
    -Wfloat-equal
    -Wformat-overflow=2
    -Wformat-signedness
    -Wformat-truncation=2
    -Winvalid-constexpr
    -Winvalid-imported-macros
    -Wmismatched-tags
    -Wmissing-declarations
    -Wmissing-include-dirs
    -Wmissing-noreturn
    -Wnoexcept
    -Wnrvo
    -Wplacement-new=2
    -Wredundant-decls
    -Wredundant-tags
    -Wshift-overflow=2
    -Wsign-promo
    -Wstrict-null-sentinel
    -Wstringop-overflow=4
    -Wsuggest-final-methods
    -Wsuggest-final-types
    -Wsuggest-override
    -Wswitch-default
    -Wswitch-enum
    -Wtrampolines
    -Wundef
    -Wuninitialized
    -Wunused-const-variable=2
    -Wunused-macros
    -Wuse-after-free=3
    -Wvla
    -Wvolatile
    -Wzero-as-null-pointer-constant)

target_compile_options(elle_project_options INTERFACE ${ELLE_GCC_WARNINGS}
                                                      $<$<BOOL:${ELLE_WARNINGS_AS_ERRORS}>:-Werror>)

if(ELLE_ENABLE_GCC_ANALYZER)
  target_compile_options(elle_project_options INTERFACE -fanalyzer -Wanalyzer-too-complex)
endif()

# clang-tidy, cppcheck and include-what-you-use run on every compilation of our own targets
# (elle_enable_analyzers). ELLE_REQUIRE_ANALYZERS turns a missing tool into a configure error, which
# the CI-style presets set so a check can never be skipped silently.

include_guard(GLOBAL)

option(ELLE_ENABLE_CLANG_TIDY "Run clang-tidy on every translation unit" ON)
option(ELLE_ENABLE_CPPCHECK "Run cppcheck on every translation unit" ON)
option(ELLE_ENABLE_IWYU "Run include-what-you-use on every translation unit" OFF)
option(ELLE_REQUIRE_ANALYZERS "Fail configuration when an enabled analyzer is not installed" OFF)

function(_elle_missing_tool name)
  if(ELLE_REQUIRE_ANALYZERS)
    message(FATAL_ERROR "${name} is enabled but was not found")
  else()
    message(WARNING "${name} is enabled but was not found; skipping it")
  endif()
endfunction()

# clang-based tools parse our GCC 16 compile commands. They must use GCC 16's libstdc++ (not the
# system's older one) and ignore GCC-only warning flags.
get_filename_component(_elle_gcc_bin "${CMAKE_CXX_COMPILER}" DIRECTORY)
get_filename_component(_elle_gcc_prefix "${_elle_gcc_bin}" DIRECTORY)
file(GLOB _elle_gcc_install_dir LIST_DIRECTORIES true
     "${_elle_gcc_prefix}/lib/gcc/*/${CMAKE_CXX_COMPILER_VERSION}")
set(_elle_clang_args --extra-arg=--gcc-install-dir=${_elle_gcc_install_dir}
                     --extra-arg=-Wno-unknown-warning-option)

if(ELLE_ENABLE_CLANG_TIDY)
  find_program(ELLE_CLANG_TIDY NAMES clang-tidy-19 clang-tidy)
  if(ELLE_CLANG_TIDY)
    set(_elle_clang_tidy_command ${ELLE_CLANG_TIDY} -p=${CMAKE_BINARY_DIR} ${_elle_clang_args}
                                 --warnings-as-errors=*)
  else()
    _elle_missing_tool(clang-tidy)
  endif()
endif()

if(ELLE_ENABLE_CPPCHECK)
  find_program(ELLE_CPPCHECK NAMES cppcheck)
  if(ELLE_CPPCHECK)
    set(_elle_cppcheck_command
        ${ELLE_CPPCHECK}
        --enable=all
        --inconclusive
        --inline-suppr
        --error-exitcode=2
        --std=c++20 # newest standard cppcheck understands; it parses C++23 code leniently
        --suppress=missingIncludeSystem
        --suppress=unmatchedSuppression
        --suppress=checkersReport
        # Reports functions unused within one translation unit; meaningless per-TU.
        --suppress=unusedFunction
        --suppressions-list=${PROJECT_SOURCE_DIR}/.cppcheck-suppressions)
  else()
    _elle_missing_tool(cppcheck)
  endif()
endif()

if(ELLE_ENABLE_IWYU)
  find_program(ELLE_IWYU NAMES include-what-you-use iwyu)
  if(ELLE_IWYU)
    set(_elle_iwyu_command ${ELLE_IWYU} -Xiwyu --error -Xiwyu --cxx17ns
                           --gcc-install-dir=${_elle_gcc_install_dir} -Wno-unknown-warning-option)
  else()
    _elle_missing_tool(include-what-you-use)
  endif()
endif()

function(elle_enable_analyzers target)
  if(_elle_clang_tidy_command)
    set_target_properties(${target} PROPERTIES CXX_CLANG_TIDY "${_elle_clang_tidy_command}")
  endif()
  if(_elle_cppcheck_command)
    set_target_properties(${target} PROPERTIES CXX_CPPCHECK "${_elle_cppcheck_command}")
  endif()
  if(_elle_iwyu_command)
    set_target_properties(${target} PROPERTIES CXX_INCLUDE_WHAT_YOU_USE "${_elle_iwyu_command}")
  endif()
endfunction()

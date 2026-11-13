cmake_minimum_required(VERSION 4.0)

project(
  elle
  VERSION 2.0.0
  DESCRIPTION "Electronic leadscrew and canned cycles for CNC-converted lathes on LinuxCNC"
  LANGUAGES CXX)

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(FATAL_ERROR "In-source builds are not allowed. Use a preset: cmake --preset dev")
endif()

option(ELLE_BUILD_TESTS "Build the test suite" ON)

include(cmake/ProjectOptions.cmake)
include(cmake/Dependencies.cmake)
include(cmake/StaticAnalyzers.cmake)
include(cmake/Packaging.cmake)

add_subdirectory(src/core)
add_subdirectory(src/linuxcnc)
add_subdirectory(src/app)
add_subdirectory(src/shell)

if(ELLE_BUILD_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()

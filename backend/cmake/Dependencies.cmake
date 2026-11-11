# Third-party code, pinned by release and verified by SHA-256. Declared SYSTEM so that the project's
# warnings and analyzers apply to our code only.

include_guard(GLOBAL)
include(FetchContent)

set(FETCHCONTENT_QUIET ON)

FetchContent_Declare(
  glaze
  URL https://github.com/stephenberry/glaze/archive/refs/tags/v9.1.0.tar.gz
  URL_HASH SHA256=55f8a675397c505f9bee6c160e1c26b0085868f185d04d7a7727531fdbff4bd4
  SYSTEM EXCLUDE_FROM_ALL)
set(glaze_DEVELOPER_MODE OFF)
set(glaze_ENABLE_FUZZING OFF)
FetchContent_MakeAvailable(glaze)

if(ELLE_BUILD_TESTS)
  FetchContent_Declare(
    Catch2
    URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.16.1.tar.gz
    URL_HASH SHA256=d16bd1b8b2364918bd472cb216394acb93bde6784890df0d91bccfb3f28d777d
    SYSTEM EXCLUDE_FROM_ALL)
  FetchContent_MakeAvailable(Catch2)
  list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
endif()

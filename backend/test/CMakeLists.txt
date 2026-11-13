include(Catch)

add_executable(elle_core_tests golden_cycles_test.cpp numbers_test.cpp cycle_errors_test.cpp
                               leadscrew_test.cpp jog_test.cpp validation_test.cpp
                               lathe_controller_test.cpp settings_test.cpp backplot_test.cpp
                               session_test.cpp task_worker_test.cpp ini_test.cpp
                               options_test.cpp task_heartbeat_test.cpp)
target_link_libraries(elle_core_tests PRIVATE elle::core elle::app elle::project_options
                                              Catch2::Catch2WithMain glaze::glaze)
target_compile_definitions(
  elle_core_tests PRIVATE ELLE_GOLDEN_CYCLES_PATH="${CMAKE_CURRENT_SOURCE_DIR}/golden/cycles.json")
elle_enable_analyzers(elle_core_tests)

catch_discover_tests(elle_core_tests)

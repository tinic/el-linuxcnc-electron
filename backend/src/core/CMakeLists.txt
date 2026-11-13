# Pure machine-independent logic: no HAL, no NML, no GUI. Everything here is unit-testable.
add_library(elle_core STATIC src/numbers.cpp src/gcode_common.cpp src/threading.cpp src/turning.cpp
                             src/leadscrew.cpp src/jog.cpp src/validation.cpp
                             src/lathe_controller.cpp src/backplot.cpp src/task.cpp
                             src/cycle_form.cpp)
add_library(elle::core ALIAS elle_core)

target_include_directories(elle_core PUBLIC include PRIVATE src)
target_link_libraries(elle_core PRIVATE elle::project_options)
elle_enable_analyzers(elle_core)

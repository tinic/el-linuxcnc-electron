# Application services between the core and the outside world: settings persistence today, the
# operator-panel protocol and the control loop as they are added. No LinuxCNC or GUI headers.
add_library(elle_app STATIC src/settings.cpp src/protocol.cpp src/session.cpp src/task_worker.cpp
                            src/ini.cpp src/options.cpp src/control_loop.cpp)
add_library(elle::app ALIAS elle_app)

target_include_directories(elle_app PUBLIC include)
target_link_libraries(
  elle_app
  PUBLIC elle::core
  PRIVATE elle::project_options glaze::glaze)
elle_enable_analyzers(elle_app)

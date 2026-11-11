# The elle executable: GTK/WebKit window plus wiring. Built only when WebKitGTK's development files
# are installed (libwebkit2gtk-4.1-dev); everything else builds and tests without them.

if(NOT TARGET elle_linuxcnc)
  return()
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(ELLE_WEBKIT IMPORTED_TARGET webkit2gtk-4.1 gtk+-3.0)

if(NOT ELLE_WEBKIT_FOUND)
  message(WARNING "webkit2gtk-4.1 not found: the elle executable is not built. "
                  "Install libwebkit2gtk-4.1-dev.")
  return()
endif()

# WebKitGTK and GTK headers are not warning-clean; only panel_window.cpp includes them, as SYSTEM.
add_library(elle_panel_window STATIC panel_window.cpp)
target_link_libraries(elle_panel_window PRIVATE elle::project_options PkgConfig::ELLE_WEBKIT)
get_target_property(_elle_webkit_includes PkgConfig::ELLE_WEBKIT INTERFACE_INCLUDE_DIRECTORIES)
set_target_properties(PkgConfig::ELLE_WEBKIT PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES
                                                        "${_elle_webkit_includes}")
elle_enable_analyzers(elle_panel_window)

add_executable(elle main.cpp)
target_link_libraries(elle PRIVATE elle::app elle::linuxcnc elle_panel_window
                                   elle::project_options)
elle_enable_analyzers(elle)

include(GNUInstallDirs)
install(TARGETS elle RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

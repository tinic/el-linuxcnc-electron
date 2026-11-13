# Thin adapters over LinuxCNC's C/C++ interfaces: the HAL component and the NML task channels.
# LinuxCNC's headers are not warning-clean, so they are included as SYSTEM headers and only from
# the .cpp files here; nothing outside this directory sees them.

find_path(ELLE_LINUXCNC_INCLUDE_DIR hal.h PATH_SUFFIXES linuxcnc)
find_library(ELLE_LINUXCNC_HAL linuxcnchal)
find_library(ELLE_LINUXCNC_NML nml)
find_library(ELLE_LINUXCNC_EMC NAMES liblinuxcnc.a linuxcnc)
find_library(ELLE_LINUXCNC_INI linuxcncini)
if(NOT (ELLE_LINUXCNC_INCLUDE_DIR AND ELLE_LINUXCNC_HAL AND ELLE_LINUXCNC_NML AND ELLE_LINUXCNC_EMC
        AND ELLE_LINUXCNC_INI))
  if(ELLE_REQUIRE_LINUXCNC)
    message(FATAL_ERROR "LinuxCNC development files not found (linuxcnc-uspace-dev)")
  endif()
  message(WARNING "LinuxCNC development files not found (linuxcnc-uspace-dev): the LinuxCNC "
                  "adapters and the elle executable are not built.")
  return()
endif()

add_library(elle_linuxcnc STATIC src/lathe_component.cpp src/task_client.cpp)
add_library(elle::linuxcnc ALIAS elle_linuxcnc)

target_include_directories(elle_linuxcnc PUBLIC include)
target_include_directories(elle_linuxcnc SYSTEM PRIVATE ${ELLE_LINUXCNC_INCLUDE_DIR})
# User-space (non-realtime) side of the RTAPI/HAL headers.
target_compile_definitions(elle_linuxcnc PRIVATE ULAPI)
target_link_libraries(
  elle_linuxcnc
  PRIVATE elle::project_options
  PUBLIC elle::core
  PUBLIC ${ELLE_LINUXCNC_EMC} ${ELLE_LINUXCNC_NML} ${ELLE_LINUXCNC_HAL} ${ELLE_LINUXCNC_INI})
elle_enable_analyzers(elle_linuxcnc)

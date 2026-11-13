# Installation layout and the Debian package.
#
#   /usr/bin/elle                       the executable (LinuxCNC DISPLAY program)
#   /usr/lib/elle/libstdc++.so.6        GCC 16's C++ runtime; Debian 12's is too old
#   /usr/lib/elle/libgcc_s.so.1         GCC 16's support library, for the same reason
#   /usr/share/elle/ui/                 the built operator panel (ui/dist)
#   /usr/share/elle/machine/            example LinuxCNC configuration (machine/)

include_guard(GLOBAL)
include(GNUInstallDirs)

set(ELLE_UI_DIST "${PROJECT_SOURCE_DIR}/../ui/dist" CACHE PATH "Built operator panel to install")
set(ELLE_MACHINE_CONFIG "${PROJECT_SOURCE_DIR}/../machine" CACHE PATH "Machine configuration")

if(EXISTS "${ELLE_UI_DIST}/index.html")
  install(DIRECTORY "${ELLE_UI_DIST}/" DESTINATION ${CMAKE_INSTALL_DATADIR}/elle/ui)
else()
  message(STATUS "ui/dist not built; the package will not contain the panel (npm run build)")
endif()
install(DIRECTORY "${ELLE_MACHINE_CONFIG}/" DESTINATION ${CMAKE_INSTALL_DATADIR}/elle/machine)

# Ship the GCC runtime the binary was built against and point the binary at it: Debian 12's
# libstdc++ and libgcc_s are too old for GCC 16 (GLIBCXX_3.4.3x, GCC_13.0.0).
foreach(_elle_runtime IN ITEMS libstdc++.so.6 libgcc_s.so.1)
  execute_process(
    COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=${_elle_runtime}
    OUTPUT_VARIABLE _elle_runtime_path
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(NOT IS_ABSOLUTE "${_elle_runtime_path}" OR NOT EXISTS "${_elle_runtime_path}")
    message(FATAL_ERROR "${CMAKE_CXX_COMPILER} does not provide ${_elle_runtime}")
  endif()
  # GCC's runtime ships with full debug information; CPACK_STRIP_FILES does not cover it.
  set(_elle_runtime_stripped "${CMAKE_BINARY_DIR}/runtime/${_elle_runtime}")
  file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/runtime")
  execute_process(COMMAND ${CMAKE_STRIP} --strip-debug -o "${_elle_runtime_stripped}"
                          "${_elle_runtime_path}" COMMAND_ERROR_IS_FATAL ANY)
  install(FILES "${_elle_runtime_stripped}" DESTINATION ${CMAKE_INSTALL_LIBDIR}/elle)
endforeach()
set(CMAKE_INSTALL_RPATH "\$ORIGIN/../${CMAKE_INSTALL_LIBDIR}/elle")

set(CPACK_GENERATOR DEB)
set(CPACK_PACKAGE_NAME elle)
set(CPACK_PACKAGE_VENDOR "Tinic Uro")
set(CPACK_PACKAGE_CONTACT "Tinic Uro")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGING_INSTALL_PREFIX /usr)
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_STRIP_FILES TRUE)
set(CPACK_DEBIAN_PACKAGE_SECTION misc)
# The binary reads LinuxCNC's NML status structures, whose layout belongs to one LinuxCNC series:
# depend on the series of the linuxcnc-uspace-dev it was built against (>= that version, < the
# next minor). And on the C library it was built with, as GCC 16 binaries use its newest symbols.
set(_elle_linuxcnc_depends "linuxcnc-uspace")
execute_process(
  COMMAND dpkg-query --show --showformat=\${Version} linuxcnc-uspace-dev
  OUTPUT_VARIABLE _elle_linuxcnc_version
  RESULT_VARIABLE _elle_dpkg_query
  ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
if(_elle_dpkg_query EQUAL 0
   AND _elle_linuxcnc_version MATCHES "^(([0-9]+:)?([0-9]+)\\.([0-9]+)[^-]*)(-[^-]*)?$")
  set(_elle_linuxcnc_upstream "${CMAKE_MATCH_1}")
  math(EXPR _elle_linuxcnc_next_minor "${CMAKE_MATCH_4} + 1")
  set(_elle_linuxcnc_depends
      "linuxcnc-uspace (>= ${_elle_linuxcnc_upstream}), linuxcnc-uspace (<< ${CMAKE_MATCH_2}${CMAKE_MATCH_3}.${_elle_linuxcnc_next_minor}~)")
else()
  message(STATUS "linuxcnc-uspace-dev is not an installed package; the package depends on any LinuxCNC")
endif()
execute_process(
  COMMAND getconf GNU_LIBC_VERSION
  OUTPUT_VARIABLE _elle_libc
  OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY)
string(REGEX REPLACE "^glibc " "" _elle_libc_version "${_elle_libc}")
set(CPACK_DEBIAN_PACKAGE_DEPENDS
    "${_elle_linuxcnc_depends}, libwebkit2gtk-4.1-0 (>= 2.40), libgtk-3-0, libc6 (>= ${_elle_libc_version})")
message(STATUS "Package depends: ${CPACK_DEBIAN_PACKAGE_DEPENDS}")
set(CPACK_DEBIAN_PACKAGE_CONFLICTS "elle-app")
include(CPack)

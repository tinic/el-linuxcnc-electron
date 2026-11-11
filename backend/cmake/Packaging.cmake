# Installation layout and the Debian package.
#
#   /usr/bin/elle                       the executable (LinuxCNC DISPLAY program)
#   /usr/lib/elle/libstdc++.so.6        GCC 16's C++ runtime; Debian 12's is too old
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

# Ship the C++ runtime the binary was built against and point the binary at it.
execute_process(
  COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=libstdc++.so.6
  OUTPUT_VARIABLE _elle_libstdcxx
  OUTPUT_STRIP_TRAILING_WHITESPACE)
if(EXISTS "${_elle_libstdcxx}")
  file(REAL_PATH "${_elle_libstdcxx}" _elle_libstdcxx_real)
  install(FILES "${_elle_libstdcxx_real}" DESTINATION ${CMAKE_INSTALL_LIBDIR}/elle
          RENAME libstdc++.so.6)
  set(CMAKE_INSTALL_RPATH "\$ORIGIN/../${CMAKE_INSTALL_LIBDIR}/elle")
endif()

set(CPACK_GENERATOR DEB)
set(CPACK_PACKAGE_NAME elle)
set(CPACK_PACKAGE_VENDOR "Tinic Uro")
set(CPACK_PACKAGE_CONTACT "Tinic Uro")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGING_INSTALL_PREFIX /usr)
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_SECTION misc)
set(CPACK_DEBIAN_PACKAGE_DEPENDS
    "linuxcnc-uspace (>= 2.9), libwebkit2gtk-4.1-0 (>= 2.40), libgtk-3-0, libc6 (>= 2.36)")
set(CPACK_DEBIAN_PACKAGE_CONFLICTS "elle-app")
include(CPack)

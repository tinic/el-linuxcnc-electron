#!/bin/sh
# Installs the build dependencies on Debian 12 with LinuxCNC's repository configured, then builds
# the operator panel and the elle package. GCC 16 must already be in ~/.local/gcc-16 (see
# tools/build-gcc16.sh); CMake 4 and Ninja are expected on PATH.
set -eu
sudo apt install -y build-essential git curl pkg-config nodejs npm \
    linuxcnc-uspace linuxcnc-uspace-dev libwebkit2gtk-4.1-dev \
    clang-tidy-19 cppcheck iwyu
cd "$(dirname "$0")/ui"
npm ci
npm run build
cd ../backend
cmake --preset release
cmake --build --preset release
ctest --preset release
cpack --config build/release/CPackConfig.cmake

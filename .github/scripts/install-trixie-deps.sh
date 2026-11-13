#!/bin/sh
# Build dependencies inside gcc:16.2.0-trixie (Debian 13, GCC 16): CMake 4, Ninja, LinuxCNC
# from linuxcnc.org (pinned: the lathe runs exactly this version, and the binary reads its NML
# structures), and WebKitGTK. Extra apt packages (for example the analyzers) are arguments.
set -eu

linuxcnc_version=1:2.9.10
here=$(cd "$(dirname "$0")" && pwd)

cmake_version=4.4.5
cmake_dir=/opt/cmake-${cmake_version}-linux-x86_64
cd /tmp
curl -fsSLO https://github.com/Kitware/CMake/releases/download/v${cmake_version}/cmake-${cmake_version}-linux-x86_64.tar.gz
curl -fsSLO https://github.com/Kitware/CMake/releases/download/v${cmake_version}/cmake-${cmake_version}-SHA-256.txt
grep "cmake-${cmake_version}-linux-x86_64.tar.gz" cmake-${cmake_version}-SHA-256.txt | sha256sum -c -
tar -xzf cmake-${cmake_version}-linux-x86_64.tar.gz -C /opt
echo "${cmake_dir}/bin" >> "${GITHUB_PATH}"

# Key 0CE3E44425B11F871BBCC6F5E43B5A8E78CC2927, "LinuxCNC Archive Signing Key".
install -m 644 "${here}/../linuxcnc-archive-keyring.gpg" /usr/share/keyrings/linuxcnc.gpg
echo "deb [arch=amd64 signed-by=/usr/share/keyrings/linuxcnc.gpg] https://www.linuxcnc.org/ trixie base 2.9-uspace" \
  > /etc/apt/sources.list.d/linuxcnc.list

export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y --no-install-recommends \
  ninja-build pkg-config dpkg-dev file \
  "linuxcnc-uspace=${linuxcnc_version}" "linuxcnc-uspace-dev=${linuxcnc_version}" \
  libwebkit2gtk-4.1-dev libgtk-3-dev libboost-dev \
  "$@"

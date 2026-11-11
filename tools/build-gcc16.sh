#!/bin/sh
# Builds GCC 16 (C and C++ only) into ~/.local/gcc-16 without root. Debian 12 ships GCC 12, which
# lacks most of the C++23 library. The source is verified against the GNU keyring.
# Runs at idle priority and away from isolated (real-time) CPUs, so a running LinuxCNC is not
# disturbed. Takes about an hour on a 6-core machine.
set -eu
VERSION=16.2.0
WORK=${WORK:-$HOME/.cache/gcc-$VERSION}
mkdir -p "$WORK" && cd "$WORK"
curl -fsSLO "https://ftp.gnu.org/gnu/gcc/gcc-$VERSION/gcc-$VERSION.tar.xz"
curl -fsSLO "https://ftp.gnu.org/gnu/gcc/gcc-$VERSION/gcc-$VERSION.tar.xz.sig"
curl -fsSL -o gnu-keyring.gpg https://ftp.gnu.org/gnu/gnu-keyring.gpg
gpgv --keyring ./gnu-keyring.gpg "gcc-$VERSION.tar.xz.sig" "gcc-$VERSION.tar.xz"
tar -xf "gcc-$VERSION.tar.xz"
(cd "gcc-$VERSION" && ./contrib/download_prerequisites)
mkdir -p build && cd build
"../gcc-$VERSION/configure" --prefix="$HOME/.local/gcc-16" --enable-languages=c,c++ \
    --disable-multilib --program-suffix=-16 --with-system-zlib --enable-checking=release
CPUS=$(cat /sys/devices/system/cpu/isolated 2>/dev/null || true)
ALLOWED=$(python3 -c "import os; iso=set();
s='$CPUS'
for part in filter(None, s.split(',')):
    a,_,b=part.partition('-'); iso.update(range(int(a), int(b or a)+1))
print(','.join(str(c) for c in sorted(os.sched_getaffinity(0)) if c not in iso))")
taskset -c "$ALLOWED" nice -n 19 ionice -c3 make -j"$(nproc)"
make install

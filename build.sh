#!/bin/bash
set -euo pipefail

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 {release|debug|profiling}"
    exit 1
fi

BUILD_TYPE="Release"
PROFILING="false"
case "$1" in
    release)
        BUILD_TYPE="Release"
        ;;
    debug)
        BUILD_TYPE="Debug"
        ;;
    profiling)
        BUILD_TYPE="Release"
        PROFILING="true"
        ;;
    *)
        echo "Invalid build type: $1"
        echo "Usage: $0 {release|debug|profiling}"
        exit 1
        ;;
esac

apt install -y libtbb-dev libmkl-dev libjemalloc-dev libboost-dev
git submodule update --init --recursive
rm -rf build
mkdir -p build
cd build
if [ "$PROFILING" = "true" ]; then
    cmake .. -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DPROFILING=ON -DROOT_PROFILING=ON -DROBIN_ARCH="${ROBIN_ARCH:-native}"
else
    cmake .. -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DROBIN_ARCH="${ROBIN_ARCH:-native}"
fi
make -j"${BUILD_JOBS:-$(nproc)}"
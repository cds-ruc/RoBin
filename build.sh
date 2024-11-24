#!/bin/bash
apt install -y libtbb-dev libmkl-dev libjemalloc-dev libboost-dev
git submodule update --init --recursive
rm -rf build
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=OPTI_LIGHT -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
make -j
#!/bin/bash

rm -rf build
mkdir build
cd build
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON .. # CMAKE_EXPORT_COMPILE_COMMANDS is needed by CodeChecker; only generates additional JSON file with compile commands.

make -j$(($(nproc)-1))
#!/bin/bash

# create and cd a directory to store the CMake build results
mkdir -p build && cd build

# configure the build tree
cmake -DCMAKE_BUILD_TYPE=Release ..

# started build
make

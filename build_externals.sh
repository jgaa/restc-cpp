#!/bin/sh

cd externals/unittest-cpp
rm -rf build
mkdir build
cd build
cmake ..
make


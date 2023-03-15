#!/usr/bin/env bash

git clone https://github.com/jgaa/restc-cpp.git
cd restc-cpp
rm -rf build
mkdir build
cd build

conan install ../conan/conanfile.txt -s build_type=Release --build
cmake -G "Unix Makefiles" ..
cmake --build . --config Release


cd ..

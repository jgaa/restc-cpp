rem Please run from the root dir
rem : .\conan\build_with_conan_vc2015.bat

rmdir /S /Q build
mkdir build
cd build
conan install -f conan/conanfile.txt -g cmake_multi --build=missing ..
cmake -G "Visual Studio 14 Win64" ..
cmake --build . --config Release

rem Debug builds currently does not work with conan built
rem boost 1.63 using shared librarie. Most of the tests
rem segfaults.
rem
rem cmake --build . --config Debug
cd ..

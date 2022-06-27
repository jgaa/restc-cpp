rem   Example file on how to build under Windows
rem   using vcpkg for x86
rem   change [path to vcpkg] to your vcpkg directory

rmdir /S /Q build
mkdir build
cd build
rem -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON
cmake -DVCPKG_TARGET_TRIPLET=x64-windows -DCMAKE_TOOLCHAIN_FILE=[path to vcpkg]/scripts/buildsystems/vcpkg.cmake -G "Visual Studio 16 2019" -A x64 ..
cmake --build . --config Debug
cmake --build . --config Release
cd ..

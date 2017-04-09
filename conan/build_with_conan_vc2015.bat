rem Please run from the root dir
rem : .\conan\build_with_conan_vc2015.bat

rmdir /S /Q build
mkdir build
cd build

conan install -f conan/conanfile.txt .. -s compiler="Visual Studio" -s compiler.version="14" -s build_type=Debug --build
cmake -G "Visual Studio 14 Win64" ..
cmake --build . --config Debug

rem conan install -f conan/conanfile.txt .. -s compiler="Visual Studio" -s compiler.version="14" -s build_type=Release --build
rem cmake -G "Visual Studio 14 Win64" ..
rem cmake --build . --config Release

cd ..

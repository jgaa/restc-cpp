cd externals\unittest-cpp
rmdir /S /Q build
mkdir build
cd build
cmake cmake -G "Visual Studio 14 Win64" ..
cmake --build . --config Debug
cmake --build . --config Release
cd ..\..\..\

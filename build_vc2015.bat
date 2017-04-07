rem   Example file on how to build under Windows
rem   using DCMAKE_PREFIX_PATH to specify where
rem   cmake's find* should look for dependencies. 

rmdir /S /Q build
mkdir build
cd build
rem -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON
cmake -DCMAKE_PREFIX_PATH=c:/devel/zlib-1.2.11;c:/devel/zlib-1.2.11/Release;c:/devel/openssl;c:/devel/boost_1_64_0 -G "Visual Studio 14 Win64" ..
cmake --build . --config Debug
cmake --build . --config Release
cd ..

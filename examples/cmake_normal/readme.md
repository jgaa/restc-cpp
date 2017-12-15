# example cmdline

This example shows how to compile a program using restc-cpp from cmake.
We assume that the library has been built and installed with "make install". It depends on the cmake configuration files from the restc-cpp library being available for cmake to find.

Currently this example is only tested under Linux.


```
jgaa@vendetta:~/src/restc-cpp/examples/cmake_normal$ rm -rf build/
jgaa@vendetta:~/src/restc-cpp/examples/cmake_normal$ mkdir build
jgaa@vendetta:~/src/restc-cpp/examples/cmake_normal$ cd build/
jgaa@vendetta:~/src/restc-cpp/examples/cmake_normal/build$ cmake ..
...
-- restc-cpp found.
-- Found ZLIB: /usr/lib/x86_64-linux-gnu/libz.so (found version "1.2.8")
-- Found OpenSSL: /usr/lib/x86_64-linux-gnu/libssl.so;/usr/lib/x86_64-linux-gnu/libcrypto.so (found version "1.1.0f")
-- Found Threads: TRUE
-- Boost version: 1.62.0
-- Found the following Boost libraries:
--   system
--   program_options
--   filesystem
--   date_time
--   context
--   coroutine
--   chrono
--   log
--   thread
--   log_setup
--   regex
--   atomic
-- Configuring done
-- Generating done
-- Build files have been written to: /home/jgaa/src/restc-cpp/examples/cmake_normal/build
jgaa@vendetta:~/src/restc-cpp/examples/cmake_normal/build$ make
Scanning dependencies of target example
[ 50%] Building CXX object CMakeFiles/example.dir/main.cpp.o
[100%] Linking CXX executable example
[100%] Built target example
```

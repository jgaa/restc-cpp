# example with cmake and restc-cpp as an external project

This example shows how to compile a program using restc-cpp from cmake, delegating the compilation of restc-cpp to cmake.

This is probably the simplest way to use the library, but the external project dependencies does add some extra time each time you run make.

Currently this example is only tested under Linux.

```sh
~$ rm -rf build/
~$ mkdir build
~$ cd build/
~$ cmake ..
~$ make
```

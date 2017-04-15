# Getting started with restc-cpp

Note that the restc-cpp wiki on github contains up to date
build instructions for different platforms. The instructions
below are generic.

Clone the repository

```sh
git clone https://github.com/jgaa/restc-cpp.git
```

Initialize the submodules
```sh
cd restc-cpp
git submodule init
git submodule update
```

Compile the library and tests
```sh
mkdir dbuild
cd dbuild
cmake ..
make
cd ..
```

At this point, you can start using the library in your own C++ projects.
You need to specify to your project the paths to where you have the
<i>incluide/restc-cpp</i> include directory, the <i>externals/rapidjson/include</i>
and the library itself (<i>./lib/librestc-cpp[D]</i>. The 'D' is present in the
library name if it is compiled for debugging.

# Embedding restc-cpp's cmake file in your cmake project
TBD

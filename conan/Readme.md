# Experimental support for the conan package manager

Please see the [Conan home page](https://conan.io/) for information regarding conan.

## Status
Currently, I am only experimenting with Conan under Windows 10 with Visual Studio 2017 Community edition

Conan is not as mature as I hoped (or at least not the packages). At the moment,
I am unable to build the dependencies (openssl, zlib and boost).

I'll see if I can get it to work reliable. If not, I'll remove the conan support for now.

## How to build restc-cpp with conan

Requirements
- Visual Studio 2015, updated to the latest version
- Git (with bash shell)
- cmake (intalled with cmake in the Path)


Requirements (for conan)
- Python 2.7 (I used native python 2.7.13, 64 bit). The OpenSSL conan builder does not work with Python 3.
- Perl (I use ActivePerl 5.24, installed with perl inthe path)

To install conan
```sh
pip install conan
```

Clone restc-cpp from github.

Build dependencies and restc-cpp

From the git shell
```sh
git submodule init
git submodule update
```

From the Visual Studio command prompt
```sh
build_externals_vc2015.bat
.\conan\build_with_conan_vc2015.bat
```

## Testing with docker installed on your Windows machine

From the git bash shell, in the projects root directory
```sh
create-and-run-containers.sh
./tests/run-tests.sh build/bin
```

## Testing with docker somewhere else

Start the containers on your Docker host.
For example, if you run Windows in a Virtual Box under Linux, and you have
Docker installed on the host machine, then you can run the containers there and
then issue the following commands in the git bash shell in Windows:

```sh
export RESTC_CPP_TEST_DOCKER_ADDRESS=10.0.2.2
./tests/run-tests.sh build/bin

```

The RESTC_CPP_TEST_DOCKER_ADDRESS environment variable specifies the IP address or
hostname to your Docker host, where the containers are available



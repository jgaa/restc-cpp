# Running the tests

In order to run the test, you must have Docker installed.
The docker containers will use localhost ports 3000, 3001, 3002, 3003. 

Create and start the Docker containers.

This will create three containers, one with a generic JSON api server,
one with nginx, configured to test authentication and redirects, and
one with squid to test proxy traversal.

Note that some of the tests use the containers, others use real
API mock services on the Internet.

## Under Linux

```sh
./create-and-run-containers.sh
```

Run the tests.

```sh
./tests/run-tests.sh
```

## Under Windows
The scripts that create and use the docker containers are modified
to work 'out of the box' under Windows. I use the cygwin shell that
comes with the windows version of git to run these scripts.

```sh
./create-and-run-containers.sh
```

Run the tests.

```sh
./tests/run-tests.sh
```

## Under MacOS

MacOS will make it’s best effort to hinder you from doing what you want.

After Docker is installed and running, I have found the commands below to work.

```sh
./create-and-run-containers.sh
```

Run the tests.
Note that you must point DYLD_LIBRARY_PATH to wherever you have the boost dynamic libraries installed. The example show where they are on my machine. 

Also note that you must “source” the command (prefix with .) in order to keep the value of DYLD_LIBRARY_PATH (what part of “export” is it that macOS does not understand?).

```sh
export DYLD_LIBRARY_PATH=/Users/jgaa/src/boost_1_60_0/stage/lib
. ./tests/run-tests.sh
```


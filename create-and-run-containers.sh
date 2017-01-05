#!/bin/bash

docker ps > /dev/null
if [ $? -eq 0 ]; then
    echo "Building and starting Docker containers for testing"
else
    echo
    echo "Cannot run docker commands. "
    echo "Please install docker or give this user access to run docker commands!"
    exit -1
fi

BASE=`pwd`

echo "JSON mock server"
cd "$BASE/tests/mock-json"
./build_and_run.sh

echo "nginx mock server"
cd "$BASE/tests/mock-nginx"
./build_and_run.sh

echo "squid mock server"
cd "$BASE/tests/mock-squid/"
./build_and_run.sh


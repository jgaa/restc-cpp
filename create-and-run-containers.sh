#!/bin/bash

pushd ci/mock-backends

docker ps > /dev/null
if [ $? -eq 0 ]; then
    echo "Building and starting Docker containers for testing"
else
    echo
    echo "Cannot run docker-compose commands. "
    echo "Please install docker-compose or give this user access to run it"
    popd
    exit -1
fi

docker compose stop
docker compose build
docker compose up -d
docker ps
popd

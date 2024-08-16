#!/bin/bash

# Check if Docker is running
docker ps > /dev/null
if [ $? -ne 0 ]; then
    echo
    echo "Cannot run docker commands. "
    echo "Please install Docker or give this user access to run it"
    popd
    exit -1
fi

# Determine the correct Docker Compose command
DOCKER_COMPOSE="docker compose"
$DOCKER_COMPOSE version > /dev/null 2>&1

if [ $? -ne 0 ]; then
    DOCKER_COMPOSE="docker-compose"
    $DOCKER_COMPOSE version > /dev/null 2>&1
    if [ $? -ne 0 ]; then
        echo "Neither 'docker compose' nor 'docker-compose' is available. Please install Docker Compose."
        popd
        exit -1
    fi
fi

echo "Using Docker Compose command: $DOCKER_COMPOSE"

# Run Docker Compose commands
pushd ci/mock-backends
$DOCKER_COMPOSE down
docker ps
popd

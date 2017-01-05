#!/bin/sh

docker stop mock-json
docker rm mock-json
docker build . -t jgaa/restc-cpp-mock-json
docker run --name restc-cpp-json -d -p 3000:80 jgaa/restc-cpp-mock-json

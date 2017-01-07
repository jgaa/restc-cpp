#!/bin/sh

docker stop restc-cpp-json
docker rm restc-cpp-json
docker build . -t jgaa/restc-cpp-mock-json
docker run --name restc-cpp-json -d -p 3000:80 jgaa/restc-cpp-mock-json

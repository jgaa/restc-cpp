#!/bin/sh

docker stop restc-cpp-nginx
docker rm restc-cpp-nginx
docker build . -t jgaa/restc-cpp-mock-nginx
docker run --name restc-cpp-nginx --link restc-cpp-json:api -d -p 3001:80  -t jgaa/restc-cpp-mock-nginx

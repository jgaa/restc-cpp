#!/bin/sh

docker stop restc-cpp-squid
docker rm restc-cpp-squid
docker build . -t jgaa/restc-cpp-mock-squid
docker run --name restc-cpp-squid --link restc-cpp-nginx:api.example.com -d -p 3003:3128  -t jgaa/restc-cpp-mock-squid



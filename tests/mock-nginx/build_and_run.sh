#!/bin/sh

docker stop nginx
docker rm nginx
docker build . -t test-nginx
docker run --name nginx --link mock-json:api  -d -p 3001:80  -t test-nginx 
docker ps

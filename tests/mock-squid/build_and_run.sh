#!/bin/sh

docker stop squid
docker rm squid
docker build . -t test-squid
docker run --name squid  --link nginx:fwd -d -p 3003:3128  -t test-squid
docker ps

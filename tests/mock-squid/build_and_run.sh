#!/bin/sh

docker stop squid
docker rm squid
docker build . -t test-squid
docker run --name squid  --link nginx:fwd -d -p 3003:3128  -t test-squid

# There is a problem during boot where squid is unable to
# resolve the linked hostname
sleep 2
docker exec -it squid service squid3 restart

docker ps

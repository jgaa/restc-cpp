# Original from https://github.com/clue/docker-json-server
# Build: docker build . -t jgaa/mockjsonserver
# Use: docker run --name mock-json  -v `pwd`/db.json:/data/db.json -d -p 3000:80 jgaa/mockjsonserver

FROM node:latest
MAINTAINER Jarle Aase <jarle@jgaa.com>

RUN npm install -g json-server

WORKDIR /data
VOLUME /data

COPY db.json /data

EXPOSE 80
ADD run.sh /run.sh
ENTRYPOINT ["bash", "/run.sh"]
CMD []

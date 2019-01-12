# Original from https://github.com/clue/docker-json-server

FROM node:latest
MAINTAINER Jarle Aase <jarle@jgaa.com>

RUN npm install -g json-server

WORKDIR /data
VOLUME /data

COPY db.json.bin /data/db.json

EXPOSE 80
ADD run.sh.bin /run.sh
ENTRYPOINT ["bash", "/run.sh"]
CMD []

FROM debian:trixie
MAINTAINER jarle@jgaa.com

RUN apt-get update \
 && DEBIAN_FRONTEND=noninteractive apt-get -y --no-install-recommends upgrade \
 && DEBIAN_FRONTEND=noninteractive apt-get -y --no-install-recommends install squid
COPY squid.conf.bin /etc/squid/squid.conf

EXPOSE 3128/tcp

RUN cat /etc/hosts

CMD squid -Nd 1 -f /etc/squid/squid.conf

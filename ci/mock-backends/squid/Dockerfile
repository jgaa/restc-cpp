FROM debian:trixie
MAINTAINER jarle@jgaa.com

RUN apt-get update \
 && DEBIAN_FRONTEND=noninteractive apt-get -y --no-install-recommends upgrade \
 && DEBIAN_FRONTEND=noninteractive apt-get -y --no-install-recommends install squid3
COPY squid.conf.bin /etc/squid3/squid.conf

EXPOSE 3128/tcp

RUN cat /etc/hosts

CMD squid3 -Nd 1 -f /etc/squid3/squid.conf

FROM debian
MAINTAINER jarle@jgaa.com

RUN apt-get update \
 && DEBIAN_FRONTEND=noninteractive apt-get upgrade -y \
 && DEBIAN_FRONTEND=noninteractive apt-get install -y squid3
COPY squid.conf.bin /etc/squid3/squid.conf
RUN systemctl enable squid3
RUN service squid3 start

EXPOSE 3128/tcp

FROM nginx
RUN rm /etc/nginx/conf.d/default.conf
COPY proxy.conf.bin /etc/nginx/conf.d/proxy.conf
COPY htpasswd.bin /etc/nginx/htpasswd
RUN mkdir -p /etc/nginx/html/upload
RUN chmod 777 /etc/nginx/html/upload



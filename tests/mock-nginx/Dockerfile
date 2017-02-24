FROM nginx
RUN rm /etc/nginx/conf.d/default.conf
COPY proxy.conf /etc/nginx/conf.d
COPY htpasswd /etc/nginx
RUN mkdir -p /etc/nginx/html/upload
RUN chmod 777 /etc/nginx/html/upload



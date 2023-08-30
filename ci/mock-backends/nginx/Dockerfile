FROM nginx
RUN rm /etc/nginx/conf.d/default.conf
COPY proxy.conf.bin /etc/nginx/conf.d/proxy.conf
COPY proxy-https.conf.bin /etc/nginx/conf.d/proxy-https.conf
COPY htpasswd.bin /etc/nginx/htpasswd
COPY test.pem /etc/nginx/ssl/test.pem
COPY test-key.pem /etc/nginx/ssl/test-key.pem
RUN mkdir -p /etc/nginx/html/upload
RUN chmod 777 /etc/nginx/html/upload



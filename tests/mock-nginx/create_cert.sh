#!/bin/bash

openssl req -x509 -newkey rsa:4096 -keyout test-key.pem -out test.pem -sha256 -days 3650 -nodes -subj "/C=XX/ST=Test/L=Test/OU=Test/O=Test/CN=localhost"

#openssl x509 -in test.pem -text

FROM alpine:latest

RUN apk add curl dos2unix

ADD couchdb-setup.sh /couchdb-setup.sh
ADD config /config

RUN dos2unix /couchdb-setup.sh

CMD sh /couchdb-setup.sh

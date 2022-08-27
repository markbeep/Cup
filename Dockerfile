FROM ubuntu:20.04

WORKDIR /app

# fixes the tzdata configuration
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && \
    apt-get install -y && \
    apt-get install make gcc libcurl4-gnutls-dev libwebsockets-dev gdb wget libcairo2-dev -y

# build Disco-C
COPY external/Disco-C/external external/Disco-C/external
COPY external/Disco-C/Makefile external/Disco-C/Makefile
COPY external/Disco-C/include external/Disco-C/include
COPY external/Disco-C/src external/Disco-C/src
RUN (cd external/Disco-C && make clean && make dev)

# build bot
COPY src src
WORKDIR src
RUN rm -f main && make

# fixes CURL from not accepting the certificate
ENV SSL_CERT_FILE="/etc/ssl/certs/ca-certificates.crt"
EXPOSE 443

CMD gdb -ex "set print thread-events off" -ex run -ex bt -q main

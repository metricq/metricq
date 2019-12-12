FROM gcc:8 AS builder
LABEL maintainer="mario.bielert@tu-dresden.de"

RUN wget https://github.com/Kitware/CMake/releases/download/v3.15.4/cmake-3.15.4-Linux-x86_64.sh \
      -q -O /tmp/cmake-install.sh \
      && chmod u+x /tmp/cmake-install.sh \
      && mkdir /usr/bin/cmake \
      && /tmp/cmake-install.sh --skip-license --prefix=/usr/bin/cmake \
      && rm /tmp/cmake-install.sh

ENV PATH="/usr/bin/cmake/bin:${PATH}"

RUN useradd -m metricq
RUN apt-get update && apt-get install -y git libprotobuf-dev protobuf-compiler build-essential libssl-dev

USER metricq
COPY --chown=metricq:metricq . /home/metricq/metricq

WORKDIR /home/metricq/metricq
RUN mkdir build

WORKDIR /home/metricq/metricq/build
RUN cmake -DCMAKE_BUILD_TYPE=Release .. && make -j 2
RUN make package


FROM ubuntu:eoan

ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y libssl1.1 libprotobuf17 tzdata

RUN useradd -m metricq
COPY --chown=metricq:metricq --from=builder /home/metricq/metricq/build/metricq-1.0.0-Linux.sh /home/metricq/metricq-1.0.0-Linux.sh

USER root
RUN /home/metricq/metricq-1.0.0-Linux.sh --skip-license --prefix=/usr
USER metricq
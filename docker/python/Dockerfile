FROM python:3-slim
LABEL maintainer="mario.bielert@tu-dresden.de"

RUN useradd -m metricq
RUN pip install virtualenv
RUN apt-get update && apt-get install -y git protobuf-compiler

USER metricq

WORKDIR /home/metricq
RUN git clone https://github.com/metricq/metricq.git
RUN virtualenv venv

WORKDIR /home/metricq/metricq
RUN . /home/metricq/venv/bin/activate && pip install .

#/bin/bash

docker-compose -f tests/docker-compose-test.yml run --rm metricq-cxx metricq-source-dummy -s amqp://admin:admin@rabbitmq-server/ --token source-dummy-test1 --metric "dummy.pets" --messages-per-chunk 10 --chunk-count 100
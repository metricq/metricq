#/bin/bash
set -e

TEST_2_SINK_ID=$(docker-compose -f tests/docker-compose-test.yml run --rm --name "test_2_sink" -d metricq-cxx metricq-sink-dummy -s amqp://admin:admin@rabbitmq-server/ --count 10 --timeout 10 -m dummy.pets)
echo "Test sink started! ID is $TEST_2_SINK_ID"
docker-compose -f tests/docker-compose-test.yml run --rm --name "test_2_source" metricq-cxx metricq-source-dummy -s amqp://admin:admin@rabbitmq-server/ --token source-dummy-test2 --metric "dummy.pets" --messages-per-chunk 10 --chunk-count 10
echo "Source run completed, checking for running sink"
(docker ps | grep -q $TEST_2_SINK_ID) && docker attach --no-stdin $TEST_2_SINK_ID
echo "Sink is done"
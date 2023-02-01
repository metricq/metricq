#/bin/bash
set -e

docker run --rm --network tests_metricq-network curlimages/curl:latest curl --retry-connrefused --retry 5 --retry-delay 10 -X PUT -H "Content-Type: application/json" http://admin:admin@couchdb-server:5984/config/source-dummy-test1 -d '{}'
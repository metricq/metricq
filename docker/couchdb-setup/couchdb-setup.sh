#!/bin/sh

# Run basic CouchDB Setup
curl --no-progress-meter --retry-connrefused --retry 5 --retry-delay 10 -X POST -H "Content-Type: application/json" http://admin:admin@couchdb-server:5984/_cluster_setup -d '{"action": "enable_single_node", "bind_address": "0.0.0.0", "username": "admin", "password": "admin"}'

# Add config database
curl --no-progress-meter --retry-connrefused --retry 5 --retry-delay 10 -X PUT "http://admin:admin@couchdb-server:5984/config"

# Add all files from config folder as new configs
for token in $(ls /config); do
  cat /config/$token | curl --no-progress-meter --retry-connrefused --retry 5 --retry-delay 10 -X PUT -H "Content-Type: application/json" "http://admin:admin@couchdb-server:5984/config/$token" -d @-
done

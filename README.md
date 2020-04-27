![BSD 3-clause](https://img.shields.io/badge/license-BSD%203--clause-blue.svg)
[![PyPI](https://img.shields.io/pypi/v/metricq)](https://pypi.org/project/metricq/)

# metricq

MetricQ is a highly-scalable, distributed metric data processing framework based on RabbitMQ.
This repository used to be the central repository, but has since been splitted into several other
repositories.

The different MetricQ language implementations can be found here:

- [C++](https://github.com/metricq/metricq-cpp)
- [Python](https://github.com/metricq/metricq-python)

The proto files of the used Protobuf definitions can be found [here](https://github.com/metricq/metricq-python).

## Setup development environemt with ```docker-compose```

Just run:

```
docker-compose -f docker-compose-development.yml up
```

This will setup:

- [Grafana Server](http://localhost:4000) (port 3000 forwarded to localhost:4000)
- [CouchDB server](http://localhost:5984) (port 5984 forwarded to localhost)
- [RabbitMQ server](http://localhost:15672/) (port 5672 and 15672 forwarded to localhost)
- [wizard frontend](http://localhost:3000/wizard/) (port 3000 forwarded to localhost)
- wizard backend (port 8000 forwarded to localhost)
- manager
- metricq-grafana (port 4000 forwarded to localhost:3001)

To run it in the background append ```-d```:

```
docker-compose -f docker-compose-development.yml up -d
```

To stop everything run:

```
docker-compose -f docker-compose-development.yml stop
```

To stop and remove everything run

```
docker-compose -f docker-compose-development.yml down
```

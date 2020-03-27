![BSD 3-clause](https://img.shields.io/badge/license-BSD%203--clause-blue.svg)
![Python package](https://github.com/metricq/metricq/workflows/Python%20package/badge.svg)
![C++ Interface](https://github.com/metricq/metricq/workflows/C++%20Interface/badge.svg)
![Code style: black](https://img.shields.io/badge/code%20style-black-000000.svg)
[![PyPI](https://img.shields.io/pypi/v/metricq)](https://pypi.org/project/metricq/)
![PyPI - Wheel](https://img.shields.io/pypi/wheel/metricq)
# metricq

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

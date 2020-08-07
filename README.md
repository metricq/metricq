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

## Cluster setup in development environment

If you follow the docker compose steps from above, there will be three running RabbitMQ instances,
but they do not form a cluster yet.

The container names will be:

- metricq_rabbitmq-server-node0_1
- metricq_rabbitmq-server-node1_1
- metricq_rabbitmq-server-node2_1


### Initialize Cluster

Once all servers are running, open a shell into the node1 rabbitmq:

```
docker exec -it metricq_rabbitmq-server-node1_1 bash
```

In that shell execute this:

```
rabbitmqctl stop_app
rabbitmqctl join_cluster rabbit@node0
rabbitmqctl start_app
exit
```

Analogous for node2 to setup a cluster with three nodes.

Until the composed services are stopped with docker-compose down, the nodes will form a cluster.

### Configure like live Cluster

- Create a user-policy with
    - Name: ManagementAsHA
    - Pattern: `management`
    - Definition: `ha-mode:	all`

### Connecting to nodes from docker network

Use the hostname `rabbitmq-server` and the client will connect to random node in the cluster.

For specific nodes, use the hostnames node0, node1, or node2.

### Connecting to nodes from host or remotely

The different RabbitMQ nodes are listening on on the network interface of their host.

- node0: 5671 / 15671
- node1: 5672 / 15672
- node2: 5673 / 15673

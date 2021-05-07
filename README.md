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

## Documentation

Given the distributed architecture of MetricQ, the documentation is scattered over several repositories and webpages:

- [The documentation of the Python implementation is a good entry point](https://metricq.github.io/metricq-python/)
- [The Wiki in this repository still contains some information](https://github.com/metricq/metricq/wiki)
- [The HTA README.md talks about some details of persistence layer](https://github.com/metricq/hta/blob/master/README.md)
- [The Management protocol documentation lists all RPC's used](https://metricq.github.io/metricq-rpc-docs/)

There are also a lot of client implementations available:

- [Tools using the Python implementation](https://github.com/metricq/metricq-python/tree/master/tools)
- [Examples for the Python implementation](https://github.com/metricq/metricq-python/tree/master/examples)
- [Tools using the C++ implementation](https://github.com/metricq/metricq-cpp/tree/master/tools)
- [Examples for the C++ implementation](https://github.com/metricq/metricq-cpp/tree/master/examples)
- [All MetricQ clients in the MetricQ GitHub organization](https://github.com/metricq?q=topic%3Ametricq-client&type=&language=)
- [MetricQ client implementations on GitHub](https://github.com/search?p=2&q=metricq-client&type=Repositories)


## Setup development environment with ```docker-compose```

> Note: During the startup, especially on the first one, errors and restarts of some services are normal! Please be patient.

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
- metricq-grafana (port 4000 forwarded to localhost:3002)
- dummy source with a metric called `dummy.source`
- hta database that stores the `dummy.source` metric.

By default, all logins are `admin` / `admin`. Do not use this dockerfile for production use!

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

### Connecting to the MetricQ network

You can now connect to the network with `amqp://admin:admin@localhost` as url and `dummy.source` as a metric. Using the examples from [metricq-python](https://github.com/metricq/metricq-python).

```
pip install ".[examples]"
./examples/metricq_sink.py --server amqp://admin:admin@localhost -m dummy.source
```

## Setup clustered development environment with ```docker-compose```

If you follow the steps from above instead with `docker-compose-cluster.yml`,
three RabbitMQ nodes will be set up.
On start, they will automatically form a cluster.

The container names will be (might be different for your specific setup):

- metricq_rabbitmq-server-node0_1
- metricq_rabbitmq-server-node1_1
- metricq_rabbitmq-server-node2_1

By default, all MetricQ agents started from the compose file will connect to
`rabbitmq-server`, which resolves to any of the three nodes.

> Note: You need to make sure to use the new BuildKit by for instance setting the ENV variable
> COMPOSE_DOCKER_CLI_BUILD to 1, or use docker-compose newer than 1.28.0-rc3.

### Configure like live Cluster

- Create a user-policy with
    - Name: ManagementAsHA
    - Pattern: `management`
    - Definition: `ha-mode:	all`

### Connecting to nodes from docker network

Use the hostname `rabbitmq-server` and the client will connect to random node in the cluster.

For specific nodes, use the hostnames `rabbitmq-node0`, `rabbitmq-node1`, or `rabbitmq-node2`.

### Connecting to nodes from host or remotely

The different RabbitMQ nodes are listening on the network interface of their host.

- rabbitmq-node0: 5671 / 15671
- rabbitmq-node1: 5672 / 15672
- rabbitmq-node2: 5673 / 15673

# Acknowledgements

This work is supported in part by the German Research Foundation (DFG) within the CRC 912 - HAEC.

# Primary Reference

Thomas Ilsche, Daniel Hackenberg, Robert Schöne, Mario Bielert, Franz Höpfner and Wolfgang E. Nagel: [MetricQ: A Scalable Infrastructure for Processing High-Resolution Time Series Data 📕](https://tu-dresden.de/zih/forschung/ressourcen/dateien/projekte/haec/metricq-a-scalable-infrastructure-for-processing-high-resolution-time-series-data) 2019 IEEE/ACM Industry/University Joint International Workshop on Data-center Automation, Analytics, and Control (DAAC), Denver, CO, USA, 2019, pp. 7-12, DOI: [`10.1109/DAAC49578.2019.00007`](https://doi.org/10.1109/DAAC49578.2019.00007).

# Additional Reference

Thomas Ilsche: [Energy Measurements of High Performance Computing Systems: From Instrumentation to Analysis 📕](https://tud.qucosa.de/api/qucosa%3A71600/attachment/ATT-0/) 2020 Doctoral dissertation TU Dresden, URN: [`urn:nbn:de:bsz:14-qucosa2-716000`](https://nbn-resolving.org/urn:nbn:de:bsz:14-qucosa2-716000)

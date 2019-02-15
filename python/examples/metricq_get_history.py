#!/usr/bin/env python3
# Copyright (c) 2018, ZIH, Technische Universitaet Dresden, Federal Republic of Germany
#
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
#     * Redistributions of source code must retain the above copyright notice,
#       this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright notice,
#       this list of conditions and the following disclaimer in the documentation
#       and/or other materials provided with the distribution.
#     * Neither the name of metricq nor the names of its contributors
#       may be used to endorse or promote products derived from this software
#       without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import asyncio
import logging
import pprint
import time

import click
import click_completion
import click_log

import metricq

logger = metricq.get_logger()

click_log.basic_config(logger)
logger.setLevel('INFO')
# Use this if we ever use threads
# logger.handlers[0].formatter = logging.Formatter(fmt='%(asctime)s %(threadName)-16s %(levelname)-8s %(message)s')
logger.handlers[0].formatter = logging.Formatter(fmt='%(asctime)s [%(levelname)-8s] [%(name)-20s] %(message)s')

click_completion.init()


async def aget_history(server, token, metric, list_metrics, list_metadata):
    client = metricq.HistoryClient(token=token, management_url=server,
                                   event_loop=asyncio.get_running_loop())
    await client.connect()
    if list_metrics:
        metrics = await client.history_metric_list(metric)
        click.echo(click.style('metrics matching {}:\n{}'.format(metric, metrics),
                               fg='bright_blue'))
        return
    if list_metadata:
        metadata = await client.history_metric_metadata(metric)
        pp = pprint.PrettyPrinter(indent=4)
        click.echo(click.style('metrics matching {}:\n{}'.format(metric, pprint.pformat(metadata)),
                               fg='bright_blue'))
        return

    start = int((time.time() - 100) * 1e9)
    end = int((time.time()) * 1e9)
    history = await client.history_data_request(metric=metric,
                                                start_time_ns=start, end_time_ns=end,
                                                interval_ns=int(1e9))
    click.echo(click.style('metric data for last 100 seconds at 1/s: {}'.format(history),
                           fg='bright_blue'))
    await client.stop()


@click.command()
@click.option('--server', default='amqp://localhost/')
@click.option('--token', default='history-py-dummy')
@click.option('--metric', default=None)
@click.option('--list-metrics', is_flag=True)
@click.option('--list-metadata', is_flag=True)
@click_log.simple_verbosity_option(logger)
def get_history(server, token, metric, list_metrics, list_metadata):
    if not (list_metrics or list_metadata) and metric is None:
        metric = 'dummy'
    asyncio.run(aget_history(server, token, metric, list_metrics, list_metadata))


if __name__ == '__main__':
    print("HALLO WELT")
    get_history()

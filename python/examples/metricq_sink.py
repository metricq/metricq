#!/usr/bin/env python3
# Copyright (c) 2019, ZIH, Technische Universitaet Dresden, Federal Republic of Germany
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

import logging

import click

import click_completion
import click_log
import metricq
from metricq.logging import get_logger

logger = get_logger()

click_log.basic_config(logger)
logger.setLevel("INFO")
logger.handlers[0].formatter = logging.Formatter(
    fmt="%(asctime)s [%(levelname)-8s] [%(name)-20s] %(message)s"
)

click_completion.init()


# To implement a MetricQ Sink, define a custom class and derive from metricq.Sink
class DummySink(metricq.Sink):

    # The constructor extracts metrics parameter
    def __init__(self, metrics, *args, **kwargs):
        logger.info("initializing DummySink")
        self._metrics = metrics
        super().__init__(*args, **kwargs)

    # We override connect() to fiddle with the connection process
    async def connect(self):
        # First, let the actual connect() happen
        await super().connect()

        # After that is done, we subscribe to the list of requested metrics
        await self.subscribe(self._metrics)

    # The data handler, this method is called for every received data point
    async def on_data(self, metric, timestamp, value):

        # For thisexample, we just print the datapoints to the console
        click.echo(
            click.style("{}: {}, {}".format(metric, timestamp, value), fg="bright_blue")
        )


@click.command()
@click.option("--server", default="amqp://localhost/")
@click.option("--token", default="sink-py-dummy")
@click.option("-m", "--metrics", multiple=True, required=True)
@click_log.simple_verbosity_option(logger)
def source(server, token, metrics):
    # initialize the DummySink class
    src = DummySink(metrics=metrics, token=token, management_url=server)

    # run the sink. This call will block until the connection is closed.
    src.run()


if __name__ == "__main__":
    source()

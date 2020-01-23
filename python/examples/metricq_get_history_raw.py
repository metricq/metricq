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
from datetime import timedelta

import click

import click_completion
import click_log
import metricq
from metricq.history_client import HistoryRequestType

logger = metricq.get_logger()

click_log.basic_config(logger)
logger.setLevel("INFO")
# Use this if we ever use threads
# logger.handlers[0].formatter = logging.Formatter(fmt='%(asctime)s %(threadName)-16s %(levelname)-8s %(message)s')
logger.handlers[0].formatter = logging.Formatter(
    fmt="%(asctime)s [%(levelname)-8s] [%(name)-20s] %(message)s"
)

click_completion.init()


async def aget_history(server, token, metric):
    client = metricq.HistoryClient(
        token=token, management_url=server, event_loop=asyncio.get_running_loop()
    )
    await client.connect()

    click.echo("connected")

    total_begin = metricq.Timestamp.from_iso8601("2020-01-01T00:00:00.0Z")
    total_end = metricq.Timestamp.now()
    chunk_duration = metricq.Timedelta.from_timedelta(timedelta(days=1))
    interval_max_raw = metricq.Timedelta(0)

    chunk_begin = total_begin
    while chunk_begin < total_end:
        chunk_end = chunk_begin + chunk_duration
        chunk_end = min(chunk_end, total_end)
        click.echo(f"Requesting chunk from {chunk_begin} to {chunk_end}")

        result = await client.history_data_request(
            metric,
            start_time=chunk_begin,
            end_time=chunk_end,
            interval_max=interval_max_raw,
            request_type=HistoryRequestType.FLEX_TIMELINE,
        )
        for tv in result.values():
            # The DB can give you one value before the requested begin timestamp
            if tv.timestamp < chunk_begin:
                continue
            click.echo(f"{tv.timestamp} {tv.value}")

        chunk_begin = chunk_end

    await client.stop(None)


@click.command()
@click.option("--server", default="amqp://localhost/")
@click.option("--token", default="history-py-dummy")
@click.argument("metric")
@click_log.simple_verbosity_option(logger)
def get_history(server, token, metric):
    asyncio.run(aget_history(server, token, metric))


if __name__ == "__main__":
    get_history()

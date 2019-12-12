# Copyright (c) 2018, ZIH,
# Technische Universitaet Dresden,
# Federal Republic of Germany
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
from abc import abstractmethod

import aio_pika
from aiormq import ChannelInvalidStateError

from .agent import PublishFailedError
from .data_client import DataClient
from .datachunk_pb2 import DataChunk
from .logging import get_logger
from .rpc import rpc_handler
from .source_metric import SourceMetric
from .types import Timestamp

logger = get_logger(__name__)


class MetricSendError(PublishFailedError):
    pass


class Source(DataClient):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.metrics = dict()
        self.chunk_size = 1

    async def connect(self):
        await super().connect()
        response = await self.rpc("source.register")
        assert response is not None
        logger.info("register response: {}", response)
        await self.data_config(**response)

        self.data_exchange = await self.data_channel.declare_exchange(
            name=response["dataExchange"], passive=True
        )

        if "config" in response:
            await self.rpc_dispatch("config", **response["config"])

        self.event_loop.create_task(self.task())

    @abstractmethod
    def task(self):
        """
        override this with your main task for generating data, e.g.
        while True:
            await self.some_read_data_function()
            await self.send(...)
        """
        pass

    def __getitem__(self, id):
        if id not in self.metrics:
            self.metrics[id] = SourceMetric(id, self, chunk_size=self.chunk_size)
        return self.metrics[id]

    async def declare_metrics(self, metrics):
        logger.debug("declare_metrics({})", metrics)
        await self.rpc("source.declare_metrics", metrics=metrics)

    async def send(self, metric, time: Timestamp, value):
        """
        Logical send.
        Dispatches to the SourceMetric for chunking
        """
        logger.debug("send({},{},{})", metric, time, value)
        metric_object = self[metric]
        assert metric_object is not None
        await metric_object.send(time, value)

    async def flush(self):
        await asyncio.gather(*[m.flush() for m in self.metrics.values() if not m.empty])

    async def _send(self, metric, data_chunk: DataChunk):
        """
        Actual send of a chunk,
        don't call from anywhere other than SourceMetric
        """
        msg = aio_pika.Message(data_chunk.SerializeToString())
        await self._data_connection_watchdog.established()
        try:
            # TOC/TOU hazard: by the time we publish, the data connection might
            # be gone again, even if we waited for it to be established before.
            await self.data_exchange.publish(msg, routing_key=metric, mandatory=False)
        except ChannelInvalidStateError as e:
            # Trying to publish on a closed channel results in a ChannelInvalidStateError
            # from aiormq.  Let's wrap that in a more descriptive error.
            raise MetricSendError(
                f"Failed to publish data chunk for metric {metric!r} "
                f"on exchange {self.data_exchange} ({self.data_connection})"
            ) from e

    @rpc_handler("config")
    async def _source_config(self, **kwargs):
        logger.info("received config {}", kwargs)

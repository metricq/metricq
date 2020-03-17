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

from abc import abstractmethod
from asyncio import CancelledError, Task
from typing import Optional, Set

import aio_pika
from aio_pika.queue import Queue
from aiormq import ChannelInvalidStateError

from .data_client import DataClient
from .datachunk_pb2 import DataChunk
from .logging import get_logger
from .types import Timestamp

logger = get_logger(__name__)


class SinkError(Exception):
    pass


class SinkResubscribeError(SinkError):
    pass


class Sink(DataClient):
    def __init__(self, *args, add_uuid=True, **kwargs):
        super().__init__(*args, add_uuid=add_uuid, **kwargs)

        self._data_queue: Optional[Queue] = None
        self._data_consumer_tag: Optional[str] = None
        self._subscribed_metrics: Set[str] = set()
        self._subscribe_args: dict = dict()
        self._resubscribe_task: Optional[Task] = None

    async def _declare_data_queue(self, name: str):
        self._data_queue = await self.data_channel.declare_queue(
            name=name, robust=False, passive=True
        )

    async def sink_config(self, dataQueue, **kwargs):
        await self.data_config(**kwargs)
        await self._declare_data_queue(dataQueue)

        logger.info("starting sink consume")
        self._data_consumer_tag = await self._data_queue.consume(self._on_data_message)

    def _on_data_connection_reconnect(self, connection):
        logger.info("Sink data connection ({}) reestablished!", connection)

        if self._resubscribe_task is not None and not self._resubscribe_task.done():
            logger.warning(
                "Sink data connection was reestablished, but another resubscribe task is still running!"
            )
            self._resubscribe_task.cancel()

        self._resubscribe_task = self.event_loop.create_task(
            self._resubscribe(connection)
        )

        def resubscribe_done(task: Task):
            try:
                exception = task.exception()
                if exception is None:
                    self._data_connection_watchdog.set_established()
                else:
                    errmsg = "Resubscription failed with an unhandled exception"
                    logger.error("{}: {}", errmsg, exception)
                    raise SinkResubscribeError(errmsg) from exception
            except CancelledError:
                logger.warning("Resubscribe task was cancelled!")

        self._resubscribe_task.add_done_callback(resubscribe_done)

    async def _resubscribe(self, connection):
        # Reuse manager-assigned data queue name for resubscription.
        self._subscribe_args.update(dataQueue=self._data_queue.name)

        metrics = tuple(self._subscribed_metrics)
        logger.info(
            "Resubscribing to {} metric(s) with RPC parameters {}...",
            len(metrics),
            self._subscribe_args,
        )
        response = await self.rpc(
            "sink.subscribe", metrics=metrics, **self._subscribe_args
        )
        await self._declare_data_queue(response["dataQueue"])

        logger.debug("Restarting consume...")
        await self._data_queue.consume(
            self._on_data_message, consumer_tag=self._data_consumer_tag
        )

    async def subscribe(self, metrics, **kwargs):
        """Subscribe to a list of metrics.

        :param metrics:
            names of the metrics to subscribe to
        :param expires:
            (optional) queue expiration time in seconds
        :param metadata: bool
            whether to return metric metadata in the response
        :return: rpc response
        """
        if self._data_queue is not None:
            kwargs["dataQueue"] = self._data_queue.name
        response = await self.rpc("sink.subscribe", metrics=metrics, **kwargs)

        self._subscribed_metrics.update(metrics)
        # Save the subscription RPC args in case we need to resubscribe (after a reconnect).
        self._subscribe_args = kwargs

        if self._data_queue is None:
            await self.sink_config(**response)
        return response

    async def unsubscribe(self, metrics):
        assert self._data_queue
        await self.rpc(
            "sink.unsubscribe", dataQueue=self._data_queue.name, metrics=metrics
        )

        self._subscribed_metrics.difference_update(metrics)

        # If we just unsubscribed from all metrics, reset the subscription args
        # to their defaults.
        if not self._subscribed_metrics:
            self._subscribe_args = dict()

    async def _on_data_message(self, message: aio_pika.IncomingMessage):
        with message.process(requeue=True):
            body = message.body
            from_token = message.app_id
            metric = message.routing_key

            logger.debug("received message from {}", from_token)
            data_response = DataChunk()
            data_response.ParseFromString(body)

            await self._on_data_chunk(metric, data_response)

    async def _on_data_chunk(self, metric, data_chunk: DataChunk):
        """ Only override this if absolutely necessary for performance """
        last_timed = 0
        zipped_tv = zip(data_chunk.time_delta, data_chunk.value)
        for time_delta, value in zipped_tv:
            last_timed += time_delta
            await self.on_data(metric, Timestamp(last_timed), value)

    @abstractmethod
    async def on_data(self, metric, timestamp, value):
        pass


class DurableSink(Sink):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, add_uuid=False, **kwargs)

    async def connect(self):
        await super().connect()

        response = await self.rpc("sink.register")
        assert response is not None
        logger.info("register response: {}", response)

        await self.rpc_dispatch("config", **response["config"])

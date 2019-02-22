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
import uuid

import aio_pika

from .logging import get_logger
from .rpc import rpc_handler
from .client import Client
from .history_pb2 import HistoryRequest, HistoryResponse

logger = get_logger(__name__)


class HistoryClient(Client):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        self.data_server_address = None
        self.history_connection = None
        self.history_channel = None
        self.history_exchange = None

        self._request_futures = dict()

    async def connect(self):
        await super().connect()
        response = await self.rpc('history.register')
        logger.info('register response: {}', response)

        self.data_server_address = self.add_credentials(response['dataServerAddress'])
        self.history_connection = await self.make_connection(self.data_server_address)
        self.history_channel = await self.history_connection.channel()
        self.history_exchange = await self.history_channel.declare_exchange(name=response['historyExchange'], passive=True)
        self.history_response_queue = await self.history_channel.declare_queue(name=response['historyQueue'], passive=True)

        if 'config' in response:
            await self.rpc_dispatch('config', **response['config'])

        await self._history_consume()

    async def stop(self, reason=None):
        logger.info('closing history channel and connection.')
        if self.history_channel:
            await self.history_channel.close()
            self.history_channel = None
        if self.history_connection:
            await self.history_connection.close()
            self.history_connection = None
        self.history_exchange = None
        await super().stop()

    # TODO refactor return type (namedtuple) and input times
    # caller should not need to know anything about the protobuf representation
    async def history_data_request(self, metric: str, start_time_ns, end_time_ns, interval_ns, timeout=60):
        logger.info('running history request for {} ({}-{},{})', metric, start_time_ns, end_time_ns, interval_ns)
        if not metric:
            raise ValueError('metric must be a non-empty string')
        correlation_id = 'mq-history-py-{}-{}'.format(self.token, uuid.uuid4().hex)
        request = HistoryRequest()
        request.start_time = start_time_ns
        request.end_time = end_time_ns
        request.interval_ns = interval_ns
        msg = aio_pika.Message(body=request.SerializeToString(),
                               correlation_id=correlation_id,
                               reply_to=self.history_response_queue.name)

        self._request_futures[correlation_id] = asyncio.Future(loop=self.event_loop)
        await self.history_exchange.publish(msg, metric)

        result = await asyncio.wait_for(self._request_futures[correlation_id], timeout=timeout)
        del self._request_futures[correlation_id]
        return result

    async def history_metric_list(self, selector=None):
        arguments = {'format': 'array'}
        if selector:
            arguments['selector'] = selector
        result = await self.rpc('history.get_metrics', **arguments)
        return result["metrics"]

    async def history_metric_metadata(self, selector=None):
        arguments = {'format': 'object'}
        if selector:
            arguments['selector'] = selector
        result = await self.rpc('history.get_metrics', **arguments)
        return result["metrics"]

    @rpc_handler('config')
    async def _history_config(self, **kwargs):
        logger.info('received config {}', kwargs)

    async def _history_consume(self, extra_queues=[]):
        logger.info('starting history consume')
        queues = [self.history_response_queue] + extra_queues
        await asyncio.wait([
            queue.consume(self._on_history_response)
            for queue in queues
        ], loop=self.event_loop)

    async def _on_history_response(self, message: aio_pika.Message):
            with message.process(requeue=True):
                body = message.body
                from_token = message.app_id
                correlation_id = message.correlation_id

                logger.info('received message from {}, correlation id: {}, reply_to: {}',
                            from_token, correlation_id, message.reply_to)
                history_response = HistoryResponse()
                history_response.ParseFromString(body)

                logger.debug('message is an history response')
                try:
                    future = self._request_futures[correlation_id]
                except KeyError:
                    logger.error('received history response with unknown correlation id {} '
                                 'from {}', correlation_id, from_token)
                    return

                future.set_result(history_response)

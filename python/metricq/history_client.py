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
from collections import namedtuple
import uuid
from enum import Enum

import aio_pika

from .logging import get_logger
from .rpc import rpc_handler
from .client import Client
from . import history_pb2
from .types import Timedelta, Timestamp, TimeValue, TimeAggregate

logger = get_logger(__name__)


class HistoryRequestType:
    AGGREGATE_TIMELINE = history_pb2.HistoryRequest.AGGREGATE_TIMELINE
    AGGREGATE = history_pb2.HistoryRequest.AGGREGATE
    LAST_VALUE = history_pb2.HistoryRequest.LAST_VALUE


class HistoryResponseType(Enum):
    AGGREGATES = 1
    VALUES = 2
    LEGACY = 3


class HistoryResponse:
    """ Currently read-only """
    def __init__(self, proto: history_pb2.HistoryResponse, request_duration=None):
        self.request_duration = request_duration
        count = len(proto.time_delta)
        if len(proto.aggregate) == count:
            self._mode = HistoryResponseType.AGGREGATES
            assert len(proto.value) == 0, 'Inconsistent HistoryResponse message'

        elif len(proto.value) == count:
            self._mode = HistoryResponseType.VALUES
            assert len(proto.aggregate) == 0, 'Inconsistent HistoryResponse message'

        elif len(proto.value_avg) == count:
            self._mode = HistoryResponseType.LEGACY
            assert len(proto.value_min) == count
            assert len(proto.value_max) == count
            assert len(proto.aggregate) == 0
            assert len(proto.value) == 0

        else:
            raise ValueError('Inconsistent HistoryResponse message')

        self._proto = proto

    def __len__(self):
        return len(self._proto.time_delta)

    @property
    def mode(self):
        return self._mode

    def values(self, convert=False):
        """
        :parameter `convert` other responses to values transparently
        :raises `ValueError` if `convert` is False and the underlying response does not contain aggregates
        :returns a Generator of `TimeValue`
        """
        time_ns = 0
        if self._mode == HistoryResponseType.VALUES:
            for time_delta, value in zip(self._proto.time_delta, self._proto.value):
                time_ns = time_ns + time_delta
                yield TimeValue(Timestamp(time_ns), value)
            return

        if not convert:
            raise ValueError(
                'Attempting to access values of HistoryResponse.values in wrong mode: {}'.format(self._mode))

        if self._mode == HistoryResponseType.AGGREGATES:
            for time_delta, proto_aggregate in zip(self._proto.time_delta, self._proto.aggregate):
                time_ns = time_ns + time_delta
                timestamp = Timestamp(time_ns)
                aggregate = TimeAggregate.from_proto(timestamp, proto_aggregate)
                yield TimeValue(timestamp, aggregate.mean)
            return

        if self._mode == HistoryResponseType.LEGACY:
            for time_delta, average in zip(self._proto.time_delta, self._proto.value_avg):
                time_ns = time_ns + time_delta
                yield TimeValue(Timestamp(time_ns), average)
            return

        raise ValueError('Invalid HistoryResponse mode')

    def aggregates(self, convert=False):
        """
        :parameter `convert` other responses to aggregates transparently
        :raises `ValueError` if convert is False and the underlying response does not contain aggregates
        :returns a Generator of `TimeAggregate`
        """
        time_ns = 0
        if self._mode == HistoryResponseType.AGGREGATES:
            for time_delta, proto_aggregate in zip(self._proto.time_delta, self._proto.aggregate):
                time_ns = time_ns + time_delta
                timestamp = Timestamp(time_ns)
                yield TimeAggregate.from_proto(timestamp, proto_aggregate)
            return

        if not convert:
            raise ValueError(
                'Attempting to access values of HistoryResponse.aggregates in wrong mode: {}'.format(self._mode))

        if self._mode == HistoryResponseType.VALUES:
            for time_delta, value in zip(self._proto.time_delta, self._proto.value):
                time_ns = time_ns + time_delta
                yield TimeAggregate.from_value(Timestamp(time_ns), value)
            return

        if self._mode == HistoryResponseType.LEGACY:
            for time_delta, minimum, maximum, average in zip(self._proto.time_delta, self._proto.value_min,
                                                             self._proto.value_max, self._proto.value_avg):
                time_ns = time_ns + time_delta
                # That of course only makes sense if you just use mean or mean_sum
                yield TimeAggregate(timestamp=Timestamp(time_ns),
                                    minimum=minimum, maximum=maximum,
                                    sum=average, count=1,
                                    integral=0, active_time=0)
            return

        raise ValueError('Invalid HistoryResponse mode')


class HistoryClient(Client):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        self.data_server_address = None
        self.history_connection = None
        self.history_channel = None
        self.history_exchange = None
        self.history_response_queue = None

        self._request_futures = dict()

    async def connect(self):
        await super().connect()
        response = await self.rpc('history.register')
        logger.debug('register response: {}', response)

        self.data_server_address = self.add_credentials(response['dataServerAddress'])
        self.history_connection = await self.make_connection(self.data_server_address)
        self.history_channel = await self.history_connection.channel()
        self.history_exchange = await self.history_channel.declare_exchange(
            name=response['historyExchange'], passive=True)
        self.history_response_queue = await self.history_channel.declare_queue(
            name=response['historyQueue'], passive=True)

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

    async def history_data_request(self, metric: str,
                                   start_time: Timestamp, end_time: Timestamp, interval_max: Timedelta,
                                   request_type: HistoryRequestType = HistoryRequestType.AGGREGATE_TIMELINE,
                                   timeout=60):
        logger.info('running history request for {} ({}-{},{})', metric, start_time, end_time, interval_max)
        if not metric:
            raise ValueError('metric must be a non-empty string')
        correlation_id = 'mq-history-py-{}-{}'.format(self.token, uuid.uuid4().hex)

        request = history_pb2.HistoryRequest()
        if start_time is not None:
            request.start_time = start_time.posix_ns
        if end_time is not None:
            request.end_time = end_time.posix_ns
        if interval_max is not None:
            request.interval_max = interval_max.ns
        if request_type is not None:
            request.type = request_type

        msg = aio_pika.Message(body=request.SerializeToString(),
                               correlation_id=correlation_id,
                               reply_to=self.history_response_queue.name)

        self._request_futures[correlation_id] = asyncio.Future(loop=self.event_loop)
        await self.history_exchange.publish(msg, metric)

        try:
            result = await asyncio.wait_for(self._request_futures[correlation_id], timeout=timeout)
        finally:
            del self._request_futures[correlation_id]
        return result

    async def history_last_value(self, metric: str, timeout=60):
        result = await self.history_data_request(metric, start_time=None, end_time=None, interval_max=None,
                                                 request_type=HistoryRequestType.LAST_VALUE, timeout=timeout)
        assert len(result) == 1
        return next(result.values())

    async def history_metric_list(self, selector=None, historic=True, timeout=None):
        arguments = {'format': 'array'}
        if selector:
            arguments['selector'] = selector
        if timeout is not None:
            arguments['timeout'] = timeout
        if historic is not None:
            arguments['historic'] = historic
        result = await self.rpc('history.get_metrics', **arguments)
        return result["metrics"]

    async def history_metric_metadata(self, selector=None, historic=True):
        arguments = {'format': 'object'}
        if selector:
            arguments['selector'] = selector
        if historic is not None:
            arguments['historic'] = historic
        result = await self.rpc('history.get_metrics', **arguments)
        return result["metrics"]

    @rpc_handler('config')
    async def _history_config(self, **kwargs):
        logger.info('received config {}', kwargs)

    async def _history_consume(self, extra_queues=[]):
        logger.info('starting history consume')
        queues = [self.history_response_queue] + extra_queues
        await asyncio.gather(*[
            queue.consume(self._on_history_response)
            for queue in queues
        ], loop=self.event_loop)

    async def _on_history_response(self, message: aio_pika.Message):
        with message.process(requeue=True):
            body = message.body
            from_token = message.app_id
            correlation_id = message.correlation_id
            request_duration = message.headers.get("x-request-duration", "-1")

            logger.info('received message from {}, correlation id: {}, reply_to: {}',
                        from_token, correlation_id, message.reply_to)
            history_response_pb = history_pb2.HistoryResponse()
            history_response_pb.ParseFromString(body)

            history_response = HistoryResponse(history_response_pb, request_duration)

            logger.debug('message is an history response')
            try:
                future = self._request_futures[correlation_id]
                future.set_result(history_response)
            except (KeyError, asyncio.InvalidStateError):
                logger.error('received history response with unknown correlation id {} '
                             'from {}', correlation_id, from_token)
                return

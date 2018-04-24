from abc import abstractmethod
import asyncio
from time import time

import aio_pika

from .logging import logger
from . import datachunk_pb2
from .rpc import rpc_handler
from .connection import Connection
from .datachunk_pb2 import DataChunk
from .source_metric import SourceMetric


class Source(Connection):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.starting_time = time()

        self.data_server_address = None
        self.data_connection = None
        self.data_channel = None
        self.data_exchange = None

        self.metrics = dict()

    async def run_task(self):
        await super().run_task()
        await self.rpc('source.register', self.handle_register_response)

    @rpc_handler('discover')
    async def handle_discover(self):
        logger.info('responding to discover')
        t = time()
        return {
            'alive': True,
            'uptime': t - self.starting_time,
            'time': t,
        }

    @rpc_handler('config')
    async def handle_config(self, config):
        pass

    async def handle_register_response(self, **response):
        logger.info('source register response: {}', response)
        assert not self.data_connection
        self.data_server_address = response['dataServerAddress']
        self.data_connection = await aio_pika.connect_robust(self.data_server_address,
                                                             loop=self.event_loop)
        self.data_channel = await self.data_connection.channel()
        self.data_exchange = await self.data_channel.declare_exchange(
            name=response['dataExchange'], passive=True)

        if 'config' in response:
            await self.dispatch('config', **response['config'])

        await self.ready_callback()
        if hasattr(self, 'run_forever'):
            asyncio.get_event_loop().create_task(self.run_forever())

        #TODO @bmario make a nice timer class

    @abstractmethod
    async def ready_callback(self):
        pass

    def __getitem__(self, id):
        if id not in self.metrics:
            self.metrics[id] = SourceMetric(id, self)
        return self.metrics[id]

    async def send(self, id, datachunk: DataChunk):
        msg = aio_pika.Message(datachunk.SerializeToString())
        await self.data_exchange.publish(msg, routing_key=id)

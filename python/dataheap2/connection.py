import asyncio

import aio_pika
from .logging import logger


class Connection:
    def __init__(self, token, rpc_url):
        self.token = token
        self.rpc_url = rpc_url
        self.management_broadcast_exchange_name = 'dh2.broadcast'

        self.management_connection = None
        self.management_channel = None
        self.management_client_queue = None

        logger.info('connection object initialized {}', "ASDASDASDASD")

    async def run(self, loop):
        logger.info("establishing rpc connection to {}", self.rpc_url)
        self.management_connection = await aio_pika.connect_robust(self.rpc_url, loop=loop)
        self.management_channel = await self.management_connection.channel()
        self.management_client_queue = await self.management_channel.declare_queue(
            'client-{}-rpc'.format(self.token), exclusive=True)

        broadcast_exchange = await self.management_channel.declare_exchange(
            name=self.management_broadcast_exchange_name, passive=True)
        await self.management_client_queue.bind(exchange=broadcast_exchange, routing_key="#")

        logger.info("starting rpc consume")
        await self.management_client_queue.consume(self.handle_management_message)

    async def handle_management_message(self, message: aio_pika.Message):
        with message.process(requeue=True):
            body_str = message.body.decode()
            token = message.properties.app_id
            logger.info('received request from {}: {}', token, body_str)

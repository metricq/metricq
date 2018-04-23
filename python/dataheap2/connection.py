import asyncio
import json

import aio_pika
from .logging import logger
from .rpc import rpc, RPCBase


class Connection(RPCBase):
    def __init__(self, token, rpc_url):
        self.token = token
        self.rpc_url = rpc_url
        self.management_broadcast_exchange_name = 'dh2.broadcast'

        self.management_connection = None
        self.management_channel = None
        self.management_client_queue = None

        self.rpc_response_handlers = dict()

        logger.info('connection object initialized')

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
            logger.info('received message from {}: {}', token, body_str)
            body = json.loads(body_str)

            handler = self.rpc_response_handlers.get(message.properties.correlation_id, None)
            if not handler:
                response = await self.dispatch(body['function'], **body)
                if response is None:
                    response = dict()
                await self.management_channel.default_exchange.publish(
                    aio_pika.Message(body=json.dumps(response).encode(),
                                     correlation_id=message.correlation_id,
                                     content_type="application/json"),
                    routing_key=message.properties.reply_to
                )
            else:
                # TODO add lots of sanity checks
                await handler(**body)


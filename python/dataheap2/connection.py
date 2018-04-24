import asyncio
import json
import uuid
import traceback

import aio_pika
from .logging import logger
from .rpc import RPCBase


def panic(loop, context):
    print("EXCEPTION: {}".format(context['message']))
    if context['exception']:
        print(context['exception'])
        traceback.print_tb(context['exception'].__traceback__)
    loop.stop()


class Connection(RPCBase):
    def __init__(self, token, management_url):
        self.token = token

        self.management_url = management_url
        self.management_broadcast_exchange_name = 'dh2.broadcast'
        self.management_exchange_name = 'dh2.management'

        self.management_broadcast_exchange = None
        self.management_exchange = None

        self.management_connection = None
        self.management_channel = None
        self.management_client_queue = None

        self.rpc_response_handlers = dict()

        self.event_loop = None

        logger.info('connection object initialized')

    def run(self):
        loop = asyncio.get_event_loop()
        loop.set_exception_handler(panic)
        loop.create_task(self.run_task())
        loop.run_forever()

    async def run_task(self):
        logger.info("establishing management connection to {}", self.management_url)

        self.management_connection = await aio_pika.connect_robust(self.management_url)
        self.management_channel = await self.management_connection.channel()
        self.management_client_queue = await self.management_channel.declare_queue(
            'client-{}-rpc'.format(self.token), exclusive=True)

        self.management_broadcast_exchange = await self.management_channel.declare_exchange(
            name=self.management_broadcast_exchange_name, passive=True
        )
        await self.management_client_queue.bind(exchange=self.management_broadcast_exchange,
                                                routing_key="#")

        self.management_exchange = await self.management_channel.declare_exchange(
            name=self.management_exchange_name, passive=True
        )

        logger.info("starting rpc consume")

        await self.management_client_queue.consume(self.handle_management_message)

    async def rpc(self, function, response_callback, payload=None):
        logger.info('management RPC {}: {}', function, payload)

        correlation_id = 'dh2-rpc-py-{}-{}'.format(self.token, uuid.uuid4().hex)
        if payload is None:
            payload = dict()
        payload['function'] = function
        body = json.dumps(payload).encode()

        msg = aio_pika.Message(body=body, correlation_id=correlation_id, app_id=self.token,
                               reply_to=self.management_client_queue.name,
                               content_type='application/json')
        self.rpc_response_handlers[correlation_id] = response_callback
        await self.management_exchange.publish(msg, routing_key=function)

    async def handle_management_message(self, message: aio_pika.Message):
        with message.process(requeue=True):
            body_str = message.body.decode()
            token = message.app_id
            correlation_id = message.correlation_id.decode()
            logger.info('received message from {}, correlation id: {}, reply_to {}\n{}',
                        token, correlation_id, message.reply_to, body_str)
            body = json.loads(body_str)

            try:
                handler = self.rpc_response_handlers.get(correlation_id, None)
                if not handler:
                    if 'function' not in body:
                        raise KeyError('no response handler and no function for RPC')
                    response = await self.dispatch(**body)
                    if response is None:
                        response = dict()
                    await self.management_channel.default_exchange.publish(
                        aio_pika.Message(body=json.dumps(response).encode(),
                                         correlation_id=correlation_id,
                                         content_type='application/json',
                                         app_id=self.token),
                        routing_key=message.reply_to
                    )
                else:
                    await handler(**body)
            except Exception:
                logger.error('failed to handle management rpc message from {}, {}\n{}',
                             token,
                             body_str, traceback.format_exc())

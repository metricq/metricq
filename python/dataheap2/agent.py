from abc import abstractmethod
import asyncio
import json
import traceback
import uuid

import aio_pika

from .logging import get_logger
from .rpc import RPCBase

logger = get_logger(__name__)

def handle_exception(loop, context):
    logger.error('exception in event loop: {}'.format(context['message']))
    try:
        logger.error('Future: {}', context['future'])
    except KeyError:
        pass
    try:
        logger.error('Handle: {}', context['handle'])
    except KeyError:
        pass
    try:
        ex = context['exception']
        logger.error('Exception: {} ({})', ex, type(ex).__qualname__)
        # TODO figure out how to logger
        traceback.print_tb(ex.__traceback__)
    except KeyError:
        pass
    # loop.stop()


class Agent(RPCBase):
    def __init__(self, token, management_url, event_loop=None):
        self.token = token

        self._event_loop = event_loop

        self._management_url = management_url
        self._management_broadcast_exchange_name = 'dh2.broadcast'
        self._management_exchange_name = 'dh2.management'

        self._management_connection = None
        self._management_channel = None

        self._management_agent_queue = None

        self._management_broadcast_exchange = None
        self._management_exchange = None

        self._rpc_response_handlers = dict()
        logger.debug('initialized')

    def make_correlation_id(self):
        return 'dh2-rpc-py-{}-{}'.format(self.token, uuid.uuid4().hex)

    @property
    def event_loop(self):
        # TODO maybe we should instead **TRY** to get the loop of the current context
        if self._event_loop is None:
            self._event_loop = asyncio.new_event_loop()
        return self._event_loop

    @event_loop.setter
    def event_loop(self, loop):
        assert self._event_loop is None
        self._event_loop = loop

    def run(self, exception_handler=handle_exception):
        if exception_handler:
            self.event_loop.set_exception_handler(exception_handler)
        self.event_loop.create_task(self.connect())
        logger.debug('running event loop {}', self.event_loop)
        self.event_loop.run_forever()
        logger.debug('run_forever completed')

    async def _connect(self, url):
        ssl = url.startswith("amqps")
        connection = await aio_pika.connect_robust(url, loop=self.event_loop, reconnect_interval=5, ssl=ssl)
        # How stupid that we can't easily add the handlers *before* actually connecting.
        # We could make our own RobustConnection object, but then we loose url parsing convenience
        connection.add_reconnect_callback(self.on_reconnect)
        connection.add_close_callback(self.on_close)
        return connection

    async def connect(self):
        logger.info("establishing management connection to {}", self._management_url)

        self._management_connection = await self._connect(self._management_url)
        self._management_channel = await self._management_connection.channel()
        self._management_agent_queue = await self._management_channel.declare_queue(
            '{}-rpc'.format(self.token), exclusive=True)

    async def _management_consume(self, extra_queues=[]):
        logger.info('starting RPC consume')
        queues = [self._management_agent_queue] + extra_queues
        await asyncio.wait([
            queue.consume(self.handle_management_message)
            for queue in queues
        ], loop=self.event_loop)

    async def _rpc(self, function, response_callback,
                   exchange: aio_pika.Exchange, routing_key: str,
                   arguments=None, timeout=60, cleanup_on_response=True):
        logger.info('sending RPC {}, exchange: {}, rk: {}, arguments: {}',
                    function, exchange.name, routing_key, arguments)

        if arguments is None:
            arguments = dict()
        arguments['function'] = function
        body = json.dumps(arguments).encode()
        correlation_id = self.make_correlation_id()
        msg = aio_pika.Message(body=body, correlation_id=correlation_id,
                               app_id=self.token,
                               reply_to=self._management_agent_queue.name,
                               content_type='application/json')
        self._rpc_response_handlers[correlation_id] = (response_callback, cleanup_on_response)
        await exchange.publish(msg, routing_key=routing_key)

        if timeout:
            def cleanup():
                try:
                    del self._rpc_response_handlers[correlation_id]
                except KeyError:
                    pass
            self.event_loop.call_later(timeout, cleanup)

    async def handle_management_message(self, message: aio_pika.Message):
        with message.process(requeue=True):
            body = message.body.decode()
            from_token = message.app_id
            correlation_id = message.correlation_id.decode()

            logger.info('received message from {}, correlation id: {}, reply_to: {}\n{}',
                        from_token, correlation_id, message.reply_to, body)
            arguments = json.loads(body)
            arguments['from_token'] = from_token

            if 'function' in arguments:
                logger.debug('message is an RPC')
                try:
                    response = await self.rpc_dispatch(**arguments)
                except Exception as e:
                    response = {'error': str(e)}
                if response is None:
                    response = dict()
                await self._management_channel.default_exchange.publish(
                    aio_pika.Message(body=json.dumps(response).encode(),
                                     correlation_id=correlation_id,
                                     content_type='application/json',
                                     app_id=self.token),
                    routing_key=message.reply_to)
            else:
                logger.debug('message is an RPC response')
                try:
                    handler, cleanup = self._rpc_response_handlers[correlation_id]
                except KeyError:
                    logger.error('received RPC response with unknown correlation id {} '
                                 'from {}', correlation_id, from_token)
                    return
                if cleanup:
                    del self._rpc_response_handlers[correlation_id]

                # Allow simple handlers that are not coroutines
                # But only None to not get any confusion
                r = handler(**arguments)
                if r is not None:
                    await r

    def on_reconnect(self, connection):
        logger.info('reconnected to {}', connection)

    def on_close(self, connection):
        logger.info('closing connection to {}', connection)

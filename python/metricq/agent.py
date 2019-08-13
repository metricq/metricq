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
import functools
import json
import signal
import ssl
import textwrap
import time
import traceback
import uuid

import aio_pika
from yarl import URL

from .logging import get_logger
from .rpc import RPCDispatcher

logger = get_logger(__name__)
timer = time.monotonic


class RPCError(RuntimeError):
    pass


class Agent(RPCDispatcher):
    LOG_MAX_WIDTH = 200

    def __init__(self, token, management_url, event_loop=None, add_uuid=False):
        self.token = f"{token}.{uuid.uuid4().hex}" if add_uuid else token

        self._event_loop_owned = False
        self._event_loop = event_loop
        self._event_loop_cancel_on_exception = False

        self._management_url = management_url
        self._management_broadcast_exchange_name = "metricq.broadcast"
        self._management_exchange_name = "metricq.management"

        self._management_connection = None
        self._management_channel = None

        self.management_rpc_queue = None

        self._management_broadcast_exchange = None
        self._management_exchange = None

        self._rpc_response_handlers = dict()
        logger.debug("initialized")

    def add_credentials(self, address):
        """ Add the credentials from the management connection to the provided address """
        management_obj = URL(self._management_url)
        address_obj = URL(address)
        return str(
            address_obj.with_user(management_obj.user).with_password(
                management_obj.password
            )
        )

    @property
    def event_loop(self):
        # TODO maybe we should instead **TRY** to get the loop of the current context
        if self._event_loop is None:
            logger.debug("Getting current or new")
            try:
                self._event_loop = asyncio.get_event_loop()
            except RuntimeError:
                # Until python 3.6, it may raise instead of creating a new one
                self._event_loop = asyncio.new_event_loop()
        return self._event_loop

    @event_loop.setter
    def event_loop(self, loop):
        assert self._event_loop is None
        self._event_loop = loop

    async def make_connection(self, url):
        if url.startswith("amqps"):
            ssl_options = {
                "cert_reqs": ssl.CERT_REQUIRED,
                "ssl_version": ssl.PROTOCOL_TLS | ssl.OP_NO_SSLv2 | ssl.OP_NO_SSLv3,
            }
        else:
            ssl_options = None
        connection = await aio_pika.connect_robust(
            url, loop=self.event_loop, reconnect_interval=30, ssl_options=ssl_options
        )
        # How stupid that we can't easily add the handlers *before* actually connecting.
        # We could make our own RobustConnection object, but then we loose url parsing convenience
        connection.add_reconnect_callback(self._on_reconnect)
        connection.add_close_callback(self._on_close)
        return connection

    async def connect(self):
        logger.info(
            "establishing management connection to {}",
            URL(self._management_url).with_password("***"),
        )

        self._management_connection = await self.make_connection(self._management_url)
        self._management_channel = await self._management_connection.channel()
        self.management_rpc_queue = await self._management_channel.declare_queue(
            "{}-rpc".format(self.token), exclusive=True
        )

    def run(self, catch_signals=("SIGINT", "SIGTERM"), cancel_on_exception=False):
        self._event_loop_owned = True
        self._event_loop_cancel_on_exception = cancel_on_exception
        self.event_loop.set_exception_handler(self.on_exception)
        for signame in catch_signals:
            try:
                self.event_loop.add_signal_handler(
                    getattr(signal, signame), functools.partial(self.on_signal, signame)
                )
            except RuntimeError as error:
                logger.warning(
                    "failed to setup signal handler for {}: {}", signame, error
                )

        self.event_loop.create_task(self.connect())
        logger.debug("running event loop {}", self.event_loop)
        self.event_loop.run_forever()
        self.event_loop.close()
        logger.debug("event loop completed")

    async def rpc(
        self,
        exchange: aio_pika.Exchange,
        routing_key: str,
        response_callback=None,
        timeout=60,
        cleanup_on_response=True,
        **kwargs,
    ):
        """
        :param function: tag of the RPC
        :param exchange:
        :param routing_key:
        :param response_callback:
        If given (not None), this function will be called with any response once it arrives
        rpc will then not wait for the response and return None
        If omitted (or None), rpc will return the (first) response instead
        :param timeout: After the timeout, a response will not be dispatched to the handler
        :param cleanup_on_response: If set, only the first response will be dispatched
        :param kwargs: any additional arguments are given to the RPC itself
        Remember that we use javaScriptSnakeCase
        :return:
        """
        if "function" not in kwargs:
            raise KeyError('all RPCs must contain a "function" argument')

        time_begin = timer()

        correlation_id = self._make_correlation_id()
        body = json.dumps(kwargs)
        logger.info(
            "sending RPC {}, ex: {}, rk: {}, ci: {}, args: {}",
            kwargs["function"],
            exchange.name,
            routing_key,
            correlation_id,
            textwrap.shorten(body, width=self.LOG_MAX_WIDTH),
        )
        msg = aio_pika.Message(
            body=body.encode(),
            correlation_id=correlation_id,
            app_id=self.token,
            reply_to=self.management_rpc_queue.name,
            content_type="application/json",
        )

        if response_callback is None:
            request_future = asyncio.Future(loop=self.event_loop)

            if not cleanup_on_response:
                # We must cleanup when we use the future otherwise we get errors
                # trying to set the future result multiple times ... after the future was
                # already evaluated
                raise TypeError(
                    "no cleanup_on_response requested while no response callback is given"
                )

            def response_callback(**response_kwargs):
                assert not request_future.done()
                logger.info("rpc completed in {} s", timer() - time_begin)
                if "error" in response_kwargs:
                    request_future.set_exception(RPCError(response_kwargs["error"]))
                else:
                    request_future.set_result(response_kwargs)

        else:
            request_future = None

        self._rpc_response_handlers[correlation_id] = (
            response_callback,
            cleanup_on_response,
        )
        await exchange.publish(msg, routing_key=routing_key)

        if timeout:

            def cleanup():
                try:
                    del self._rpc_response_handlers[correlation_id]
                except KeyError:
                    pass

            if not request_future:
                self.event_loop.call_later(timeout, cleanup)

        if request_future:
            try:
                return await asyncio.wait_for(request_future, timeout=timeout)
            except TimeoutError as te:
                logger.error(
                    "timeout when waiting for RPC response future {}", correlation_id
                )
                cleanup()
                raise te

    async def rpc_consume(self, extra_queues=[]):
        """
        Start consuming RPCs
        Typically this is called at the end of connect() once the Agent is prepared
        to handle RPCs
        :param extra_queues: additional queues on which to receive RPCs
        """
        logger.info("starting RPC consume")
        queues = [self.management_rpc_queue] + extra_queues
        await asyncio.gather(
            *[queue.consume(self._on_management_message) for queue in queues],
            loop=self.event_loop,
        )

    def on_signal(self, signal):
        logger.info("received signal {}, calling stop()", signal)
        self.event_loop.create_task(self.stop())

    def on_exception(self, loop, context):
        logger.error("exception in event loop: {}".format(context["message"]))
        try:
            logger.error("Future: {}", context["future"])
        except KeyError:
            pass
        try:
            logger.error("Handle: {}", context["handle"])
        except KeyError:
            pass
        try:
            ex = context["exception"]
            if isinstance(ex, KeyboardInterrupt):
                logger.info("stopping Agent on KeyboardInterrupt")
                loop.create_task(self.stop())
            else:
                logger.error("Exception: {} ({})", ex, type(ex).__qualname__)
                # TODO figure out how to logger
                traceback.print_tb(ex.__traceback__)
                if self._event_loop_cancel_on_exception:
                    logger.error("stopping Agent on unhandled exception")
                    loop.create_task(self.stop())
        except KeyError:
            pass

    async def stop(self):
        logger.info("closing management channel and connection.")
        if self._management_channel:
            await self._management_channel.close()
            self._management_channel = None
        if self._management_connection:
            await self._management_connection.close()
            self._management_connection = None
        self._management_broadcast_exchange = None
        self._management_exchange = None

        if self._event_loop_owned:
            try:
                logger.debug(
                    "remaining tasks when stopping event loop {}",
                    asyncio.all_tasks(self.event_loop),
                )
            except AttributeError:
                # needs python 3.7
                pass
            self.event_loop.stop()
        else:
            logger.debug(
                "stop completed, we do not own the event loop, so it is not stopped"
            )

    def _make_correlation_id(self):
        return "metricq-rpc-py-{}-{}".format(self.token, uuid.uuid4().hex)

    async def _on_management_message(self, message: aio_pika.Message):
        """
        :param message: This is either an RPC or an RPC response
        """
        with message.process(requeue=True):
            time_begin = timer()
            body = message.body.decode()
            from_token = message.app_id
            correlation_id = message.correlation_id

            logger.info(
                "received message from {}, correlation id: {}, reply_to: {}, length: {}\n{}",
                from_token,
                correlation_id,
                message.reply_to,
                len(body),
                textwrap.shorten(body, width=self.LOG_MAX_WIDTH),
            )
            arguments = json.loads(body)
            arguments["from_token"] = from_token

            if "function" in arguments:
                logger.debug("message is an RPC")
                try:
                    response = await self.rpc_dispatch(**arguments)
                except Exception as e:
                    logger.error(
                        "error handling RPC {} ({}): {}",
                        arguments["function"],
                        type(e),
                        traceback.format_exc(),
                    )
                    response = {"error": str(e)}
                if response is None:
                    response = dict()
                duration = timer() - time_begin
                body = json.dumps(response)
                logger.info(
                    "rpc response to {}, correlation id: {}, length: {}, time: {} s\n{}",
                    from_token,
                    correlation_id,
                    len(body),
                    duration,
                    textwrap.shorten(body, width=self.LOG_MAX_WIDTH),
                )
                await self._management_channel.default_exchange.publish(
                    aio_pika.Message(
                        body=body.encode(),
                        correlation_id=correlation_id,
                        content_type="application/json",
                        app_id=self.token,
                    ),
                    routing_key=message.reply_to,
                )
            else:
                logger.debug("message is an RPC response")
                try:
                    handler, cleanup = self._rpc_response_handlers[correlation_id]
                except KeyError:
                    logger.error(
                        "received RPC response with unknown correlation id {} from {}",
                        correlation_id,
                        from_token,
                    )
                    # We do not throw here, no requeue for this!
                    return
                if cleanup:
                    del self._rpc_response_handlers[correlation_id]

                if not handler:
                    return

                # Allow simple handlers that are not coroutines
                # But only None to not get any confusion
                r = handler(**arguments)
                if r is not None:
                    await r

    def _on_reconnect(self, connection):
        logger.info("reconnected to {}", connection)

    def _on_close(self, connection):
        logger.info("closing connection to {}", connection)

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

from datetime import datetime
from typing import Optional, Sequence, Union

from .agent import Agent, RpcRequestError
from .logging import get_logger
from .rpc import rpc_handler
from .types import Timedelta, Timestamp

logger = get_logger(__name__)


class ManagementRpcPublishError(RpcRequestError):
    pass


class Client(Agent):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        self.starting_time = datetime.now()

    @property
    def name(self):
        return "client-" + self.token

    async def connect(self):
        await super().connect()

        self._management_broadcast_exchange = await self._management_channel.declare_exchange(
            name=self._management_broadcast_exchange_name, passive=True
        )
        self._management_exchange = await self._management_channel.declare_exchange(
            name=self._management_exchange_name, passive=True
        )

        await self.management_rpc_queue.bind(
            exchange=self._management_broadcast_exchange, routing_key="#"
        )

        await self.rpc_consume()

    async def rpc(self, function, **kwargs):
        logger.debug("Waiting for management connection to be reestablished...")
        await self._management_connection_watchdog.established()
        try:
            return await super().rpc(
                function=function,
                exchange=self._management_exchange,
                routing_key=function,
                cleanup_on_response=True,
                **kwargs,
            )
        except RpcRequestError as e:
            raise ManagementRpcPublishError(
                f"Failed to send management RPC request {function!r}"
            ) from e

    @rpc_handler("discover")
    async def _on_discover(self, **kwargs):
        logger.info("responding to discover")
        t = datetime.now()
        return {
            "alive": True,
            "uptime": Timedelta.from_timedelta(t - self.starting_time).ns,
            "time": Timestamp.from_datetime(t).posix_ns,
        }

    async def get_metrics(
        self,
        selector: Union[str, Sequence[str], None] = None,
        metadata: bool = True,
        historic: Optional[bool] = None,
        timeout: Optional[float] = None,
        prefix: Optional[str] = None,
        infix: Optional[str] = None,
        limit: Optional[int] = None,
    ) -> Union[Sequence[str], Sequence[dict]]:
        """
        :param selector: regex for partial matching the metric name or sequence of possible metric names
        :param historic: filter by historic flag
        :param metadata: if true, metadata is included in response
        :param timeout: timeout for the RPC in seconds
        :param prefix: filter results by prefix on the key
        :param infix: filter results by infix on the key
        :param limit: limit the number of results to return
        :return: either a {name: metadata} dict (metadata=True) or a list of metric names (metadata=False)
        """
        arguments = {"format": "object" if metadata else "array"}
        if selector is not None:
            arguments["selector"] = selector
        if timeout is not None:
            arguments["timeout"] = timeout
        if historic is not None:
            arguments["historic"] = historic
        if prefix is not None:
            arguments["prefix"] = prefix
        if infix is not None:
            arguments["infix"] = infix
        if limit is not None:
            arguments["limit"] = limit

        # Note: checks are done in the manager (e.g. must not have prefix and historic/selector at the same time)

        result = await self.rpc("get_metrics", **arguments)
        return result["metrics"]

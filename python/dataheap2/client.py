from .logging import logger
from .agent import Agent


class Client(Agent):
    @property
    def name(self):
        return 'client-' + self.token

    async def connect(self):
        await super().connect()

        self._management_broadcast_exchange = await self._management_channel.declare_exchange(
            name=self._management_broadcast_exchange_name, passive=True)
        self._management_exchange = await self._management_channel.declare_exchange(
            name=self._management_exchange_name, passive=True)

        await self._management_agent_queue.bind(
            exchange=self._management_broadcast_exchange, routing_key="#")

        await self._management_consume()

    async def rpc(self, function, response_callback, **kwargs):
        await self._rpc(function, response_callback,
                        exchange=self._management_exchange, routing_key=function,
                        cleanup_on_response=True, **kwargs)

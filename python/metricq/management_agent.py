import aio_pika
from aiocouch import CouchDB

from .agent import Agent
from .logging import get_logger
from .rpc import rpc_handler
from .types import Timedelta, Timestamp

logger = get_logger(__name__)


class ManagementAgent(Agent):
    def __init__(
        self, token, management_url, couchdb_url, couchdb_user, couchdb_password
    ):
        super().__init__(token, management_url)

        self.couchdb_client = CouchDB(
            couchdb_url,
            user=couchdb_user,
            password=couchdb_password,
            loop=self.event_loop,
        )
        self.couchdb_db_config = None
        self.couchdb_db_metadata = None

    async def connect(self):
        # First, connect to couchdb
        self.couchdb_db_config = await self.couchdb_client.create(
            "config", exists_ok=True
        )
        self.couchdb_db_metadata = await self.couchdb_client.create(
            "metadata", exists_ok=True
        )

        # After that, we do the MetricQ connection stuff
        await super().connect()

        logger.info("creating rpc exchanges")
        self._management_exchange = await self._management_channel.declare_exchange(
            name=self._management_exchange_name,
            type=aio_pika.ExchangeType.TOPIC,
            durable=True,
        )
        self._management_broadcast_exchange = await self._management_channel.declare_exchange(
            name=self._management_broadcast_exchange_name,
            type=aio_pika.ExchangeType.FANOUT,
            durable=True,
        )

import asyncio
import logging
from time import time

import dataheap2
from dataheap2.logging import get_logger

logger = get_logger(__name__)


class TestSource(dataheap2.Source):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.period = None

    @dataheap2.rpc_handler('config')
    async def handle_config(self, **config):
        dataheap2.logger.info('test config {}', config)
        self.period = 1 / config['frequency']

    async def ready_callback(self):
        logger.info('TestSource ready')

    async def run_forever(self):
        while True:
            await self['dummyMetric'].send(time(), 42)
            await asyncio.sleep(self.period)


if __name__ == '__main__':
    ch = logging.StreamHandler()
    ch.setLevel(logging.DEBUG)

    logger.setLevel(logging.DEBUG)
    logger.addHandler(ch)

    src = TestSource('pyTestSource', 'amqp://localhost')
    src.run()

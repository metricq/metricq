import logging
import time

from dataheap2 import SynchronousSource, get_logger

logger = get_logger(__name__)


def run_source(ssource):
    for i in range(100):
        ssource.send('foo', time.time(), i)
        time.sleep(0.1)
    ssource.stop()


if __name__ == '__main__':
    logging.basicConfig(level=logging.INFO)

    ssource = SynchronousSource('pySynchronousTestSource', 'amqp://localhost')
    run_source(ssource)

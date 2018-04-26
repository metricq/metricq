import logging
import time

from dataheap2 import SynchronousSource, logger


if __name__ == '__main__':
    formatter = logging.Formatter(fmt='%(asctime)s %(threadName)-16s %(levelname)-8s %(message)s',
                                  datefmt='%Y-%m-%d %H:%M:%S')

    ch = logging.StreamHandler()
    ch.setLevel(logging.DEBUG)
    ch.setFormatter(formatter)

    logger.setLevel(logging.DEBUG)
    logger.addHandler(ch)

    sources = [
        SynchronousSource('pySynchronousTestSource{}'.format(i), 'amqp://localhost')
        for i in range(5)
    ]

    for i in range(100):
        sources[i % len(sources)].send('foo', time.time(), i)

    for source in sources:
        source.stop()

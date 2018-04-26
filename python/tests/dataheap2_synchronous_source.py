import logging
import time

from dataheap2 import SynchronousSource, logger


def run_source(ssource):
    for i in range(100):
        ssource.send('foo', time.time(), i)
        time.sleep(0.1)
    ssource.stop()


if __name__ == '__main__':
    formatter = logging.Formatter(fmt='%(asctime)s %(threadName)-16s %(levelname)-8s %(message)s',
                                  datefmt='%Y-%m-%d %H:%M:%S')

    ch = logging.StreamHandler()
    ch.setLevel(logging.DEBUG)
    ch.setFormatter(formatter)

    logger.setLevel(logging.DEBUG)
    logger.addHandler(ch)

    ssource = SynchronousSource('pySynchronousTestSource', 'amqp://localhost')
    run_source(ssource)

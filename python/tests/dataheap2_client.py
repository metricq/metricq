#!/bin/env python3
know
import asyncio

import aiomonitor
import dataheap2
import logging

if __name__ == "__main__":
    ch = logging.StreamHandler()
    ch.setLevel(logging.DEBUG)

    dataheap2.logger.setLevel(logging.DEBUG)
    dataheap2.logger.addHandler(ch)

    loop = asyncio.get_event_loop()
    c = dataheap2.Client("pytest", "amqp://localhost")
    loop.create_task(c.run(loop))
    with aiomonitor.start_monitor(loop, locals={'connection': c}):
        loop.run_forever()

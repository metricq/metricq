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
from threading import Event, Lock, Thread

from .logging import get_logger
from .source import Source

logger = get_logger(__name__)


class _SynchronousSource(Source):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        # Remember this is a threading.Event, which is threadsafe
        # not a asyncio.Event which is not threadsafe
        # Because we use threads anyway
        # There is no threading.Future
        self.exception = None
        self._ready_event = Event()

    async def connect(self):
        await super().connect()
        self._ready_event.set()

    def on_exception(self, loop, context):
        super().on_exception(loop, context)

        if not self._ready_event.is_set():
            self.exception = context["exception"]
            self._ready_event.set()

    async def task(self):
        # Nothing to do, we are called from the outside
        pass

    def wait_for_ready(self, timeout):
        if not self._ready_event.wait(timeout):
            raise TimeoutError("SynchronousSource not ready in time")
        if self.exception is not None:
            logger.error(
                "[_SynchronousSource] failed to wait for ready: {}", self.exception
            )
            raise self.exception

    def run(self):
        super().run(catch_signals=())


class SynchronousSource:
    _lock = Lock()
    _tid = 0

    def __init__(self, *args, **kwargs):
        self._source = _SynchronousSource(*args, **kwargs)
        self._thread = Thread(target=self._source.run)

        with self._lock:
            thread_id = SynchronousSource._tid
            SynchronousSource._tid += 1

        # MetricQ Synchronous Source Event Loop Thread
        self._thread.name = "MQSSELT#{}".format(thread_id)
        self._thread.start()
        logger.debug("[SynchronousSource] spawning new thread {}", self._thread.name)
        try:
            self._source.wait_for_ready(60)
        except Exception as e:
            self.stop()
            raise e

        logger.info("[SynchronousSource] ready")

    def send(self, metric, time, value, block=True, timeout=60):
        f = asyncio.run_coroutine_threadsafe(
            self._source.send(metric, time, value), self._source.event_loop
        )
        if block:
            exception = f.exception(timeout)
            if exception:
                logger.error("[SynchronousSource] failed to send data {}", exception)
                # Keep going for reconnect. If you want to panic, do the following instead
                # self.stop()
                # raise exception

    def declare_metrics(self, metrics, block=True, timeout=60):
        f = asyncio.run_coroutine_threadsafe(
            self._source.declare_metrics(metrics), self._source.event_loop
        )
        if block:
            exception = f.exception(timeout)
            if exception:
                logger.error("[SynchronousSource] failed to send data {}", exception)

    def stop(self, timeout=60):
        logger.info("[SynchronousSource] stopping")
        f = asyncio.run_coroutine_threadsafe(self._source.stop(), self._source.event_loop)
        exception = f.exception(timeout=timeout)
        if exception:
            logger.error("[SynchronousSource] stop call failed {}", exception)

        logger.debug("[SynchronousSource] underlying source stopped")
        self._thread.join()
        logger.info("[SynchronousSource] thread joined")

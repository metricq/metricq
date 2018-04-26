import asyncio
from threading import Thread, Lock, Event
import traceback

from .source import Source
from .logging import get_logger
from .agent import handle_exception

logger = get_logger(__name__)


class _SynchronousSource(Source):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        # Remember this is a threading.Event, which is threadsafe
        # not a asyncio.Event which is not threadsafe
        # Because we use threads anyway
        self.exception = None
        self._ready_event = Event()

    async def ready_callback(self):
        await super().ready_callback()
        self.event_loop.set_exception_handler(handle_exception)
        self._ready_event.set()

    def _panic(self, loop, context):
        handle_exception(loop, context)

        self.exception = context['exception']
        self._ready_event.set()

    def wait_for_ready(self, timeout):
        if not self._ready_event.wait(timeout):
            raise TimeoutError('SynchronousSource not ready in time')
        if self.exception is not None:
            logger.error('[_SynchronousSource] failed to wait for ready: {}', self.exception)
            raise self.exception

    def run(self):
        super().run(exception_handler=self._panic)


class SynchronousSource:
    _lock = Lock()
    _tid = 0

    def __init__(self, *args, **kwargs):
        self._source = _SynchronousSource(*args, **kwargs)
        self._thread = Thread(target=self._source.run)

        with self._lock:
            thread_id = SynchronousSource._tid
            SynchronousSource._tid += 1

        # DataHeap2 Synchronous Source Event Loop Thread
        self._thread.name = 'DH2SSELT#{}'.format(thread_id)
        self._thread.start()
        logger.debug('[SynchronousSource] spawning new thread {}', self._thread.name)
        try:
            self._source.wait_for_ready(60)
        except Exception as e:
            self.stop()
            raise e

        logger.info('[SynchronousSource] ready')

    def send(self, id, time, value):
        f = asyncio.run_coroutine_threadsafe(
            self._source.send(id, time, value),
            self._source.event_loop
        )
        exception = f.exception(60)
        if exception:
            logger.error('[SynchronousSource] failed to send data {}', exception)
            # Keep going for reconnect. If you want to panic, do the following instead
            # self.stop()
            # raise exception

    def stop(self):
        logger.info('[SynchronousSource] stopping')
        self._source.event_loop.stop()
        logger.debug('[SynchronousSource] event loop stopped')
        self._thread.join()
        logger.info('[SynchronousSource] thread joined')

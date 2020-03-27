from asyncio import CancelledError, Event, Task, TimeoutError, wait_for
from typing import Callable, Optional, Union

from .logging import get_logger

logger = get_logger(__name__)


class ConnectionWatchdog:
    def __init__(
        self,
        on_timeout_callback: Callable[["ConnectionWatchdog"], None],
        timeout: Union[int, float] = 60,
        connection_name: str = "connection",
    ):
        """Watch a connection, fire a callback if it failed to reconnect before
        the given timeout.

        This class wraps a watchdog task that asynchronously waits for
        established/closed events.  Use :py:meth:`start` to start the
        connection watchdog.

        :param on_timeout_callback:
            Function called when the connection failed to reconnect before the
            timeout occurs.
        :param timeout: Union[int, float]
            Time duration given until the connection is considered to have
            failed to reconnect.  Use :py:meth:`set_established` to signal
            reconnection.
        :param connection_name: str
            Human readable name of the connection, used in log messages.
        """
        self.connection_name = connection_name
        self.timeout = timeout

        self._callback = on_timeout_callback

        # Events take the loop, which we don't have here so we can't initialize them here
        self._closed_event: Optional[Event] = None
        self._established_event: Optional[Event] = None
        self._watchdog_task: Optional[Task] = None

    def start(self, loop):
        """Start the connection watchdog task.

        A call to this method will have no effect if the task is already
        running.
        """
        if self._watchdog_task:
            logger.warning(
                "ConnectionWatchdog for {} already started", self.connection_name
            )
            return

        self._closed_event = Event(loop=loop)
        self._established_event = Event(loop=loop)

        async def watchdog():
            logger.debug("Started {} watchdog", self.connection_name)
            try:
                cap_connection_name = self.connection_name.capitalize()
                while True:
                    try:
                        await wait_for(
                            self._established_event.wait(), timeout=self.timeout
                        )
                        logger.debug("{} established", cap_connection_name)
                    except TimeoutError:
                        logger.warning(
                            "{} failed to reconnect after {} seconds",
                            cap_connection_name,
                            self.timeout,
                        )
                        self._callback(self)
                        break

                    await self._closed_event.wait()
                    logger.debug("{} was closed", cap_connection_name)

            except CancelledError:
                logger.debug("Cancelled {} watchdog", self.connection_name)
                raise

        self._watchdog_task = loop.create_task(watchdog())

    def set_established(self):
        """Signal that the connection has been established.
        """
        assert (
            self._closed_event is not None
            and self._established_event is not None
            and self._watchdog_task is not None
        ), "attempting to operate with a watchdog that is not yet started"
        self._closed_event.clear()
        self._established_event.set()

    def set_closed(self):
        """Signal that the connection has been closed.
        Note: Can be called when the watchdog is already stopped, so we need to check here
        """
        if self._closed_event is None:
            return
        self._established_event.clear()
        self._closed_event.set()

    async def closed(self):
        """Asynchronously wait for the connection to be closed.
        """
        await self._closed_event.wait()

    async def established(self):
        """Asynchronously wait for the connection to be established.
        """
        await self._established_event.wait()

    async def stop(self):
        """Stop the connection watchdog task if it is running.
        """
        if self._watchdog_task:
            if self._watchdog_task.done():
                try:
                    logger.warn(
                        "Watchdog task {} already done with result: {}",
                        self.connection_name,
                        self._watchdog_task.result(),
                    )
                except Exception as e:
                    logger.error(
                        "Watchdog task {} already done with exception: {}",
                        self.connection_name,
                        e,
                    )

            self._watchdog_task.cancel()

            try:
                await self._watchdog_task
            except CancelledError:
                logger.debug("Stopping {} watchdog complete", self.connection_name)
            self._watchdog_task = None
            self._established_event = None
            self._closed_event = None

from asyncio import CancelledError, Event, Task, TimeoutError, ensure_future, wait_for
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
        self._closed_event = Event()
        self._established_event = Event()
        self._watchdog_task: Optional[Task] = None

    def start(self):
        """Start the connection watchdog task.

        A call to this method will have no effect if the task is already
        running.
        """
        if self._watchdog_task:
            logger.warning(
                f"ConnectionWatchdog for {self.connection_name} already started"
            )
            return

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
                return

        self._watchdog_task = ensure_future(watchdog())

    def set_established(self):
        """Signal that the connection has been established.
        """
        self._closed_event.clear()
        self._established_event.set()

    def set_closed(self):
        """Signal that the connection has been closed.
        """
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
            self._watchdog_task.cancel()
            await self._watchdog_task
            self._watchdog_task = None

# Copyright (c) 2018, ZIH, Technische Universitaet Dresden, Federal Republic of Germany
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

from typing import Optional

from yarl import URL

from .agent import ReconnectTimeoutError
from .client import Client
from .connection_watchdog import ConnectionWatchdog
from .logging import get_logger

logger = get_logger(__name__)


class DataClient(Client):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        self.data_server_address = None
        self.data_connection = None
        self.data_channel = None
        self.data_exchange = None
        self._data_connection_watchdog = ConnectionWatchdog(
            on_timeout_callback=lambda watchdog: self._schedule_stop(
                ReconnectTimeoutError(
                    f"Failed to reestablish {watchdog.connection_name} after {watchdog.timeout} seconds"
                )
            ),
            timeout=kwargs.get("connection_timeout", 60),
            connection_name="data connection",
        )

    async def data_config(self, dataServerAddress, **kwargs):
        """
        You should not call this in child classes because it is a registered RPC handler
        """
        logger.debug("data_config(dataServerAddress={})", dataServerAddress)
        if not dataServerAddress:
            raise ValueError(
                "invalid dataServerAddress provided: {}".format(dataServerAddress)
            )
        dataServerAddress = self.add_credentials(dataServerAddress)
        if self.data_connection:
            if dataServerAddress != self.data_server_address:
                logger.error(
                    "attempting to change dataServerAddress on the fly, not supported."
                )
            logger.info("ignoring new config")
        else:
            logger.info(
                "setting up data connection to {}",
                URL(dataServerAddress).with_password("***"),
            )
            self.data_server_address = dataServerAddress
            self.data_connection = await self.make_connection(self.data_server_address)

            self.data_connection.add_close_callback(self._on_data_connection_close)
            self.data_connection.add_reconnect_callback(
                self._on_data_connection_reconnect
            )

            # publisher confirms seem to be buggy, disable for now
            self.data_channel = await self.data_connection.channel(
                publisher_confirms=False
            )
            # TODO configurable prefetch count
            await self.data_channel.set_qos(prefetch_count=400)

            self._data_connection_watchdog.start(loop=self.event_loop)
            self._data_connection_watchdog.set_established()

    async def stop(self, exception: Optional[Exception] = None):
        logger.info("closing data channel and connection.")
        await self._data_connection_watchdog.stop()
        if self.data_channel:
            await self.data_channel.close()
            self.data_channel = None
        if self.data_connection:
            # We need not pass anything as exception to this close. It will only hurt.
            await self.data_connection.close()
            self.data_connection = None
        self.data_exchange = None
        await super().stop(exception)

    def _on_data_connection_close(self, _exception: Optional[Exception]):
        self._data_connection_watchdog.set_closed()

    def _on_data_connection_reconnect(self, _connection):
        self._data_connection_watchdog.set_established()

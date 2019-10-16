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

import asyncio
from abc import abstractmethod
from typing import Optional

from .logging import get_logger
from .source import Source
from .types import Timedelta, Timestamp

logger = get_logger(__name__)


class IntervalSource(Source):
    def __init__(self, *args, period=None, **kwargs):
        """
        :param period: in seconds
        """
        super().__init__(*args, **kwargs)
        self.period = period
        self._interval_task_stop_future = None

    async def task(self):
        self._interval_task_stop_future = self.event_loop.create_future()
        deadline = Timestamp.now()
        while True:
            await self.update()
            try:
                if self.period is None:
                    raise ValueError(
                        "IntervalSource.period not set before running task"
                    )
                deadline += Timedelta.from_s(self.period)
                now = Timestamp.now()
                while now >= deadline:
                    logger.warn("Missed deadline {}, it is now {}", deadline, now)
                    deadline += Timedelta.from_s(self.period)

                timeout = (deadline - now).s
                await asyncio.wait_for(
                    asyncio.shield(self._interval_task_stop_future), timeout=timeout
                )
                self._interval_task_stop_future.result()
                logger.info("stopping IntervalSource task")
                break
            except asyncio.TimeoutError:
                # This is the normal case, just continue with the loop
                continue

    async def stop(self, exception: Optional[Exception]):
        logger.debug("stop()")
        if self._interval_task_stop_future is not None:
            self._interval_task_stop_future.set_result(None)
        await super().stop(exception)

    @abstractmethod
    async def update(self):
        pass

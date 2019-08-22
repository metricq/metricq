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

import math

from .datachunk_pb2 import DataChunk
from .types import Timestamp


class SourceMetric:
    def __init__(self, id, source, chunk_size=1):
        self.id = id
        self.source = source

        self.chunk_size = chunk_size
        self.previous_timestamp = 0
        self.chunk = DataChunk()

    def append(self, time: Timestamp, value):
        """
        Like send, but synchronous and will never flush
        """
        timestamp = time.posix_ns
        self.chunk.time_delta.append(timestamp - self.previous_timestamp)
        self.previous_timestamp = timestamp
        self.chunk.value.append(value)

        assert len(self.chunk.time_delta) == len(self.chunk.value)

    async def send(self, time: Timestamp, value):
        self.append(time, value)
        if 0 < self.chunk_size <= len(self.chunk.time_delta):
            await self.flush()

    async def error(self, time: Timestamp):
        await self.send(time, math.nan)

    @property
    def empty(self):
        assert len(self.chunk.time_delta) == len(self.chunk.value)
        return len(self.chunk.time_delta) == 0

    async def flush(self):
        assert len(self.chunk.time_delta) == len(self.chunk.value)
        if len(self.chunk.time_delta) == 0:
            return

        await self.source._send(self.id, self.chunk)
        del self.chunk.time_delta[:]
        del self.chunk.value[:]
        self.previous_timestamp = 0

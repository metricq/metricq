from .datachunk_pb2 import DataChunk
from .types import to_timestamp


class SourceMetric:
    def __init__(self, id, source):
        self.id = id
        self.source = source

        self.chunk_size = 1
        self.previous_timestamp = 0
        self.chunk = DataChunk()

    async def send(self, time, value):
        timestamp = to_timestamp(time)
        self.chunk.time_delta.append(timestamp - self.previous_timestamp)
        self.previous_timestamp = timestamp
        self.chunk.value.append(value)

        assert len(self.chunk.time_delta) == len(self.chunk.value)
        if 0 < self.chunk_size == len(self.chunk.time_delta):
            await self.flush()

    async def flush(self):
        await self.source._send(self.id, self.chunk)
        del self.chunk.time_delta[:]
        del self.chunk.value[:]
        self.previous_timestamp = 0

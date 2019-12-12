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

import datetime
import re
from functools import total_ordering
from numbers import Number
from typing import NamedTuple, Union

from . import history_pb2


@total_ordering
class Timedelta:
    @staticmethod
    def from_timedelta(delta):
        seconds = (delta.days * 24 * 3600) + delta.seconds
        microseconds = seconds * 1000000 + delta.microseconds
        return Timedelta(microseconds * 1000)

    @staticmethod
    def from_string(duration_str: str):
        m = re.fullmatch(r"\s*([+-]?\d*[.,]?\d+)\s*([^\d]*)\s*", duration_str)
        if not m:
            raise ValueError(
                'invalid duration string {}, not of form "number unit"'.format(
                    duration_str
                )
            )
        value = float(m.group(1))
        unit = m.group(2)
        if unit in ("", "s", "second", "seconds"):
            return Timedelta(int(value * 1_000_000_000))
        if unit in ("ms", "millisecond", "milliseconds"):
            return Timedelta(int(value * 1_000_000))
        if unit in ("us", "microsecond", "microseconds", "μs"):
            return Timedelta(int(value * 1_000))
        if unit in ("ns", "nanosecond", "nanoseconds"):
            return Timedelta(int(value))
        if unit in ("min", "minute", "minutes"):
            return Timedelta(int(value * 1_000_000_000 * 60))
        if unit in ("h", "hour", "hours"):
            return Timedelta(int(value * 1_000_000_000 * 3600))
        if unit in ("d", "day", "days"):
            return Timedelta(int(value * 1_000_000_000 * 3600 * 24))
        raise ValueError("invalid duration unit {}".format(unit))

    @staticmethod
    def from_us(value: Number):
        return Timedelta(int(value * 1e3))

    @staticmethod
    def from_ms(value: Number):
        return Timedelta(int(value * 1e6))

    @staticmethod
    def from_s(value: Number):
        return Timedelta(int(value * 1e9))

    def __init__(self, value: int):
        """
        :param value: integer duration in nanoseconds
        """
        self._value = value

    @property
    def ns(self):
        return self._value

    @property
    def us(self):
        return self._value / 1e3

    @property
    def ms(self):
        return self._value / 1e6

    @property
    def s(self):
        return self._value / 1e9

    @property
    def timedelta(self):
        microseconds = self._value // 1000
        return datetime.timedelta(microseconds=microseconds)

    def __add__(self, other: Union["Timedelta", "Timestamp", datetime.timedelta]):
        if isinstance(other, Timedelta):
            return Timedelta(self._value + other._value)
        if isinstance(other, datetime.timedelta):
            return self + Timedelta.from_timedelta(other)
        # Fallback to Timestamp.__add__
        return other + self

    def __sub__(self, other: Union["Timedelta", "Timestamp", datetime.timedelta]):
        if isinstance(other, Timedelta):
            return Timedelta(self._value - other._value)
        if isinstance(other, datetime.timedelta):
            return self - Timedelta.from_timedelta(other)
        raise TypeError(
            "invalid type to subtract from Timedelta: {}".format(type(other))
        )

    def __truediv__(self, factor):
        return Timedelta(self._value // factor)

    def __mul__(self, factor):
        return Timedelta(self._value * factor)

    def __str__(self):
        return "{}s".format(self.s)

    def __eq__(self, other: Union["Timedelta", datetime.timedelta]):
        if isinstance(other, datetime.timedelta):
            return self.timedelta == other
        return self._value == other._value

    def __lt__(self, other: Union["Timedelta", datetime.timedelta]):
        if isinstance(other, datetime.timedelta):
            return self.timedelta < other
        return self._value < other._value


@total_ordering
class Timestamp:
    _EPOCH = datetime.datetime(1970, 1, 1, tzinfo=datetime.timezone.utc)

    @classmethod
    def from_posix_seconds(cls, seconds):
        return Timestamp(int(seconds * 1e9))

    @classmethod
    def from_datetime(cls, dt: datetime.datetime):
        """
        :param dt: Must be an aware datetime object
        :return:
        """
        delta = dt - Timestamp._EPOCH
        seconds = (delta.days * 24 * 3600) + delta.seconds
        microseconds = seconds * 1000000 + delta.microseconds
        return Timestamp(microseconds * 1000)

    @classmethod
    def from_iso8601(cls, iso_string: str):
        return cls.from_datetime(
            datetime.datetime.strptime(iso_string, "%Y-%m-%dT%H:%M:%S.%fZ").replace(
                tzinfo=datetime.timezone.utc
            )
        )

    @classmethod
    def now(cls):
        return cls.from_datetime(datetime.datetime.now(datetime.timezone.utc))

    def __init__(self, value: int):
        """
        :param value: integer posix timestamp in nanoseconds
        """
        self._value = value

    @property
    def posix_ns(self):
        return self._value

    @property
    def posix_us(self):
        return self._value / 1000

    @property
    def posix_ms(self):
        return self._value / 1000000

    @property
    def posix(self):
        return self._value / 1000000000

    @property
    def datetime(self):
        """
        This creates an aware UTC datetime object.
        We know in MetricQ that timestamps are POSIX timestamps, hence UTC.
        """
        # We use timedelta in the hope that this doesn't break
        # on non-POSIX systems, where fromtimestamp apparently may omit leap seconds
        # but our MetricQ timestamps are true UNIX timestamps without leap seconds
        microseconds = self._value // 1000
        return Timestamp._EPOCH + datetime.timedelta(microseconds=microseconds)

    def __add__(self, delta: Timedelta):
        return Timestamp(self._value + delta.ns)

    def __sub__(self, other: Union["Timedelta", "Timestamp"]):
        if isinstance(other, Timedelta):
            return Timestamp(self._value - other.ns)
        if isinstance(other, Timestamp):
            return Timedelta(self._value - other._value)
        raise TypeError(
            "Invalid type to subtract from Timestamp: {}".format(type(other))
        )

    def __lt__(self, other: "Timestamp"):
        return self._value < other._value

    def __eq__(self, other: "Timestamp"):
        return self._value == other._value

    def __str__(self):
        # Note we convert to local timezone with astimezone for printing
        return "[{}] {}".format(self.posix_ns, str(self.datetime.astimezone()))

    def __repr__(self):
        return str(self.posix_ns)


class TimeValue(NamedTuple):
    timestamp: Timestamp
    value: float


class TimeAggregate(NamedTuple):
    timestamp: Timestamp
    minimum: float
    maximum: float
    sum: float
    count: int
    # TODO maybe convert to 1s based integral (rather than 1ns)
    integral: float
    # TODO maybe convert to Timedelta
    active_time: int

    @staticmethod
    def from_proto(timestamp: Timestamp, proto: history_pb2.HistoryResponse.Aggregate):
        return TimeAggregate(
            timestamp=timestamp,
            minimum=proto.minimum,
            maximum=proto.maximum,
            sum=proto.sum,
            count=proto.count,
            integral=proto.integral,
            active_time=proto.active_time,
        )

    @staticmethod
    def from_value(timestamp: Timestamp, value: float):
        return TimeAggregate(
            timestamp=timestamp,
            minimum=value,
            maximum=value,
            sum=value,
            count=1,
            integral=0,
            active_time=0,
        )

    @staticmethod
    def from_value_pair(
        timestamp_before: Timestamp, timestamp: Timestamp, value: float
    ):
        assert timestamp > timestamp_before
        delta = timestamp - timestamp_before
        return TimeAggregate(
            timestamp=timestamp_before,
            minimum=value,
            maximum=value,
            sum=value,
            count=1,
            integral=delta.ns * value,
            active_time=delta.ns,
        )

    @property
    def mean(self):
        if self.active_time > 0:
            return self.mean_integral
        else:
            return self.mean_sum

    @property
    def mean_integral(self):
        return self.integral / self.active_time

    @property
    def mean_sum(self):
        return self.sum / self.count

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
from abc import ABCMeta
from collections.abc import Awaitable


class RPCMeta(ABCMeta):
    def __new__(mcs, name, bases, attrs, **kwargs):
        rpc_handlers = dict()
        for base in bases:
            try:
                rpc_handlers.update(base._rpc_handlers)
            except AttributeError:
                pass

        for fun in attrs.values():
            try:
                rpc_handlers[getattr(fun, '__rpc_tag')] = fun
            except AttributeError:
                pass

        attrs['_rpc_handlers'] = rpc_handlers
        return super().__new__(mcs, name, bases, attrs)


class RPCBase(metaclass=RPCMeta):
    async def rpc_dispatch(self, function, **kwargs):
        if function not in self._rpc_handlers:
            raise KeyError('Missing rpc handler for {}'.format(function))
        task = self._rpc_handlers[function](self, **kwargs)
        if not isinstance(task, Awaitable):
            raise TypeError('RPC handler for {} is not a coroutine'.format(function))
        return await task


def rpc_handler(tag):
    def decorator(fun):
        fun.__rpc_tag = tag
        return fun
    return decorator

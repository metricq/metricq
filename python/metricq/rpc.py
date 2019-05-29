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
from collections import defaultdict
from collections.abc import Awaitable


class RPCMeta(ABCMeta):
    """
    The created classes will have an _rpc_handlers attribute which contains
    lists of handlers for each rpc tag.
    In each list, the base-class rpc handlers will be before the child class ones
    """

    def __new__(mcs, name, bases, attrs, **kwargs):
        rpc_handlers = defaultdict(list)
        for base in bases:
            try:
                for function_tag, handlers in base._rpc_handlers.items():
                    rpc_handlers[function_tag] += handlers
            except AttributeError:
                pass

        for handler in attrs.values():
            try:
                function_tags = getattr(handler, "__rpc_tags")
                for function_tag in function_tags:
                    rpc_handlers[function_tag].append(handler)
            except AttributeError:
                # oops, not an rpc handler
                pass

        attrs["_rpc_handlers"] = rpc_handlers
        return super().__new__(mcs, name, bases, attrs)


class RPCDispatcher(metaclass=RPCMeta):
    async def rpc_dispatch(self, function, **kwargs):
        """
        Dispatches an incoming (or fake) RPC to all handlers, beginning with the base class handlers
        return values are only allowed for unique RPC handlers.
        Only keyword arguments are supported in RPCs
        :param function the tag of the function to be called.
        WARNING: DO NOT RENAME. It must be called function because it is called directly with the json dict
        """
        if function not in self._rpc_handlers:
            raise KeyError("Missing rpc handler for {}".format(function))

        for handler in self._rpc_handlers[function]:
            task = handler(self, **kwargs)
            if not isinstance(task, Awaitable):
                raise TypeError(
                    "RPC handler for {} is not a coroutine".format(function)
                )
            rv = await task
            if len(self._rpc_handlers[function]) == 1:
                return rv
            elif rv is not None:
                raise TypeError(
                    "multiple RPC handlers attempting to return a non-none value which is not permitted."
                )


def rpc_handler(*function_tags):
    """
    Decorator for an RPC handler, may contain multiple functions
    """

    def decorator(handler):
        handler.__rpc_tags = function_tags
        return handler

    return decorator

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

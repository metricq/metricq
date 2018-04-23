class RPCMeta(type):
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
    async def dispatch(self, tag, *args, **kwargs):
        if tag not in self._rpc_handlers:
            raise KeyError('Missing rpc handler for {}'.format(tag))
        return await self._rpc_handlers[tag](self, *args, **kwargs)


def rpc(tag):
    def decorator(fun):
        fun.__rpc_tag = tag
        return fun
    return decorator

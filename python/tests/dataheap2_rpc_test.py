import unittest

from dataheap2.rpc import RPCBase, rpc


class RPCSimple(RPCBase):
    def __init__(self, number):
        self.number = number

    @rpc("test")
    def handle_test(self):
        return self.number

    @rpc("toast")
    def handle_toast(self, name):
        return self.number * name


class RPCSub(RPCSimple):
    def __init__(self, number):
        self.number = number

    @rpc("sub")
    def handle_sub(self):
        return self.number * 2


class RPCOther(RPCBase):
    @rpc("foo")
    def handle_foo(self):
        return "foo"


class TestRPC(unittest.TestCase):
    def test_basic(self):
        x = RPCSimple(1)
        self.assertEqual(x.dispatch('test'), 1)
        self.assertEqual(x.dispatch('toast', 'x'), 'x')

    def test_separate(self):
        xx = RPCSimple(2)
        self.assertEqual(xx.dispatch('test'), 2)
        self.assertEqual(xx.dispatch('toast', 'x'), 'xx')

    def test_sub(self):
        s = RPCSub(3)
        self.assertEqual(s.dispatch('test'), 3)
        self.assertEqual(s.dispatch('toast', 'x'), 'xxx')
        self.assertEqual(s.dispatch('sub'), 6)

    def test_other(self):
        o = RPCOther()
        self.assertEqual(o.dispatch('foo'), 'foo')
        with self.assertRaises(KeyError):
            o.dispatch('test')

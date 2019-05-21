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
import unittest

from metricq.rpc import RPCBase, rpc_handler


class RPCSimple(RPCBase):
    def __init__(self, number):
        self.number = number

    @rpc_handler("test")
    def handle_test(self):
        return self.number

    @rpc_handler("toast")
    def handle_toast(self, name):
        return self.number * name


class RPCSub(RPCSimple):
    def __init__(self, number):
        self.number = number

    @rpc_handler("sub")
    def handle_sub(self):
        return self.number * 2


class RPCOther(RPCBase):
    @rpc_handler("foo")
    def handle_foo(self):
        return "foo"


class TestRPC(unittest.TestCase):
    def test_basic(self):
        x = RPCSimple(1)
        self.assertEqual(x.dispatch("test"), 1)
        self.assertEqual(x.dispatch("toast", "x"), "x")

    def test_separate(self):
        xx = RPCSimple(2)
        self.assertEqual(xx.dispatch("test"), 2)
        self.assertEqual(xx.dispatch("toast", "x"), "xx")

    def test_sub(self):
        s = RPCSub(3)
        self.assertEqual(s.dispatch("test"), 3)
        self.assertEqual(s.dispatch("toast", "x"), "xxx")
        self.assertEqual(s.dispatch("sub"), 6)

    def test_other(self):
        o = RPCOther()
        self.assertEqual(o.dispatch("foo"), "foo")
        with self.assertRaises(KeyError):
            o.dispatch("test")

#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""

import unittest

from common_func.decode_tool.byte_interpreter import ByteInterpreter


class TestTraceViewer(unittest.TestCase):
    def test_check(self) -> None:
        try:
            byte_interpreter = ByteInterpreter(3, [10, 12])
        except ValueError:
            pass

    def test_decode_byte(self) -> None:
        byte_interpreter = ByteInterpreter(3, [12, 12])
        self.assertEqual(tuple(byte_interpreter.decode_byte(b'\x01\x00\x10')), (1, 256))

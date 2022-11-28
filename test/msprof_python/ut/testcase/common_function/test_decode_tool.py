#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""

import unittest

from common_func.decode_tool.decode_tool import DecodeTool


class TestTraceViewer(unittest.TestCase):
    def test_decode_byte(self) -> None:
        decode_tool = DecodeTool()
        self.assertEqual(tuple(decode_tool.decode_byte("Ht", b'\x01\x00\x01\x00\x10')), (1, 1, 256))

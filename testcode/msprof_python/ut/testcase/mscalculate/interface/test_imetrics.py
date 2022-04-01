#!/usr/bin/env python
# coding=utf-8
"""
function: test imetrics
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""

import unittest
from unittest import mock

from mscalculate.interface.imetrics import IMetrics


class TestIMetrics(unittest.TestCase):
    def test_get_division(self: any) -> None:
        divisor = 15
        dividend = 123
        res = 0.12195121951219512
        self.assertEqual(IMetrics.get_division(divisor, dividend), res)

    def test_get_mul(self: any) -> None:
        value1 = 5
        value2 = 10
        self.assertEqual(IMetrics.get_mul(value1, value2), 50.0)


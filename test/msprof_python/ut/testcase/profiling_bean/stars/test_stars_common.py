#!/usr/bin/python3
# -*- coding: utf-8 -*-
#  Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
import unittest

from profiling_bean.stars.stars_common import StarsCommon


class TestStarsCommon(unittest.TestCase):

    def test_should_stream_change_low_12_bits_when_stream_bit13_is_one(self):
        stars_common = StarsCommon(8, 8195, 0)
        self.assertEqual(8, stars_common.stream_id)

    def test_should_return_stream_when_stream_bit13_is_not_one(self):
        stars_common = StarsCommon(8, 3, 0)
        self.assertEqual(3, stars_common.stream_id)

    def test_should_return_stream_low_12_bits_when_stream_bit12_is_one(self):
        stars_common = StarsCommon(8, 12291, 0)
        self.assertEqual(3, stars_common.stream_id)

    def test_should_return_task_when_stream_bit13_is_one(self):
        stars_common = StarsCommon(8, 8195, 0)
        self.assertEqual(3, stars_common.task_id)

    def test_should_task_change_low_12_bits_when_stream_bit12_and_bit13_is_not_one(self):
        stars_common = StarsCommon(8, 3, 0)
        self.assertEqual(8, stars_common.task_id)

    def test_should_return_task_change_high_3_bits_when_stream_bit12_is_one(self):
        stars_common = StarsCommon(8, 20483, 0)
        self.assertEqual(16392, stars_common.task_id)

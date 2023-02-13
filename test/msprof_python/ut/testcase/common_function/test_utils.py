#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.import unittest
import unittest

from common_func.utils import Utils


class TestUtils(unittest.TestCase):

    def test_data_processing_with_decimals_should_return_origin_data_when_data_is_empty(self):
        ret = Utils.data_processing_with_decimals([])
        self.assertEqual([], ret)

    def test_data_processing_with_decimals_should_return_origin_data_when_data_not_two_dimensional(self):
        origin_data = [1, 2, 3.4446]
        ret = Utils.data_processing_with_decimals(origin_data)
        self.assertEqual(origin_data, ret)

    def test_data_processing_with_decimals_should_return_data_with_4_decimals_when_data_has_decimals_over_4(self):
        origin_data = [[1, 2, 3.444689, 0.25390, "a"]]
        expect_data = [['1', '2', '3.4447', '0.2539', "a"]]
        ret = Utils.data_processing_with_decimals(origin_data)
        self.assertEqual(expect_data, ret)

    def test_is_valid_num(self):
        res = Utils.is_valid_num(None)
        self.assertEqual(res, 0)

    def test_percentile(self):
        data = [1, 2, 3, 4, 5, 6, 7, 8]
        ret = Utils.percentile(data, 0.33)
        self.assertEqual(3, ret)

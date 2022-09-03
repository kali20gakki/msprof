#!/usr/bin/env python
# coding=utf-8
"""
function: test l2_cache_calculator_model
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""

import unittest
from unittest import mock
from msmodel.l2_cache.l2_cache_calculator_model import L2CacheCalculatorModel
from common_func.db_name_constant import DBNameConstant

NAMESPACE = 'msmodel.l2_cache.l2_cache_calculator_model'


class TestL2CacheParserModel(unittest.TestCase):
    def test_construct(self):
        check = L2CacheCalculatorModel('test')
        self.assertEqual(check.result_dir, 'test')
        self.assertEqual(check.db_name, DBNameConstant.DB_L2CACHE)
        self.assertEqual(check.table_list, [DBNameConstant.TABLE_L2CACHE_SUMMARY])

    def test_flush(self):
        with mock.patch(NAMESPACE + '.L2CacheCalculatorModel.insert_data_to_db'):
            check = L2CacheCalculatorModel('test')
            check.flush([1])

    def test_split_events_data(self):
        self.assertEqual(L2CacheCalculatorModel.split_events_data([]), [])
        check_data = [('127.0.0.1', 0, 0, 3, 2, '640,3136,0,0,47,3776,0,3159'),
                      ('127.0.0.1', 0, 0, 3, 3, '5524,3136,0,0,5313,8660,0,3151'),
                      ('127.0.0.1', 0, 0, 3, 4, '3432,784,0,0,3425,4216,0,791'),
                      ('127.0.0.1', 0, 0, 3, 5, '896,784,0,0,870,1680,0,794'),
                      ('127.0.0.1', 0, 0, 3, 6, '1601,784,0,0,1436,2385,0,805')]
        check = L2CacheCalculatorModel.split_events_data(check_data)
        res_data_ok = [['127.0.0.1', 0, 0, 3, 2, '640', '3136', '0', '0', '47', '3776', '0', '3159'],
                       ['127.0.0.1', 0, 0, 3, 3, '5524', '3136', '0', '0', '5313', '8660', '0', '3151'],
                       ['127.0.0.1', 0, 0, 3, 4, '3432', '784', '0', '0', '3425', '4216', '0', '791'],
                       ['127.0.0.1', 0, 0, 3, 5, '896', '784', '0', '0', '870', '1680', '0', '794'],
                       ['127.0.0.1', 0, 0, 3, 6, '1601', '784', '0', '0', '1436', '2385', '0', '805']]
        self.assertEqual(check, res_data_ok)

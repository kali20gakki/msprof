#!/usr/bin/env python
# coding=utf-8
"""
function: test l2_cache_parser_model
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""

import unittest
from unittest import mock
from msmodel.l2_cache.l2_cache_parser_model import L2CacheParserModel
from common_func.db_name_constant import DBNameConstant
from sqlite.db_manager import DBManager

NAMESPACE = 'msmodel.l2_cache.l2_cache_parser_model'


class TestL2CacheParserModel(unittest.TestCase):
    def test_construct(self):
        check = L2CacheParserModel('test', [DBNameConstant.TABLE_L2CACHE_PARSE])
        self.assertEqual(check.result_dir, 'test')
        self.assertEqual(check.db_name, DBNameConstant.DB_L2CACHE)
        self.assertEqual(check.table_list, [DBNameConstant.TABLE_L2CACHE_PARSE])

    def test_flush(self):
        with mock.patch(NAMESPACE + '.L2CacheParserModel.insert_data_to_db'):
            check = L2CacheParserModel('test', [DBNameConstant.TABLE_L2CACHE_PARSE])
            check.flush(1)

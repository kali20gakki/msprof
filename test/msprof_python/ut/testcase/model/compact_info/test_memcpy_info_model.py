#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

import unittest
from unittest import mock
from msmodel.compact_info.memcpy_info_model import MemcpyInfoModel
from common_func.db_name_constant import DBNameConstant

NAMESPACE = 'msmodel.compact_info.memcpy_info_model'


class TestMemcpyInfoModel(unittest.TestCase):
    def test_construct(self):
        check = MemcpyInfoModel('test')
        self.assertEqual(check.result_dir, 'test')
        self.assertEqual(check.db_name, DBNameConstant.DB_RUNTIME)
        self.assertEqual(check.table_list, [DBNameConstant.TABLE_MEMCPY_INFO])

    def test_flush(self):
        with mock.patch(NAMESPACE + '.MemcpyInfoModel.insert_data_to_db'):
            check = MemcpyInfoModel('test')
            check.flush(['memcpy_info', 'runtime', 270722, 16, 75838931021372, 125544, 1])

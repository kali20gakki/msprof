#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

import unittest
from unittest import mock

from common_func.db_name_constant import DBNameConstant
from msmodel.compact_info.hccl_op_info_model import HcclOpInfoModel

NAMESPACE = 'msmodel.compact_info.hccl_op_info_model'


class TestHcclOpInfoModel(unittest.TestCase):

    def test_construct(self):
        check = HcclOpInfoModel('test')
        self.assertEqual(check.result_dir, 'test')
        self.assertEqual(check.db_name, DBNameConstant.DB_HCCL_INFO)
        self.assertEqual(check.table_list, [DBNameConstant.TABLE_HCCL_OP_INFO])

    def test_flush(self):
        with mock.patch(NAMESPACE + '.HcclOpInfoModel.insert_data_to_db'):
            check = HcclOpInfoModel('test')
            check.flush([[
                'node', 'hccl_op_info', 353695, 38202863941896, 0, 1, 'FP16', 'NHR-HD', 43642, '72840854065026987'
            ]])

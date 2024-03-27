#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.

import unittest
from unittest import mock

from common_func.db_name_constant import DBNameConstant
from msmodel.compact_info.node_attr_info_model import NodeAttrInfoModel

NAMESPACE = 'msmodel.compact_info.node_attr_info_model'


class TestNodeAttrInfoModel(unittest.TestCase):
    def test_construct(self):
        check = NodeAttrInfoModel('test')
        self.assertEqual(check.result_dir, 'test')
        self.assertEqual(check.db_name, DBNameConstant.DB_NODE_ATTR_INFO)
        self.assertEqual(check.table_list, [DBNameConstant.TABLE_NODE_ATTR_INFO])

    def test_flush(self):
        with mock.patch(NAMESPACE + '.NodeAttrInfoModel.insert_data_to_db'):
            check = NodeAttrInfoModel('test')
            check.flush(['node', 'node_attr_info', 3602526, 38202864071026,
                         'strides: 0; pads: 0; dilations: 0; groups: 1;data_formats: 0'])




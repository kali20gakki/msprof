#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
"""
import unittest
from unittest import mock

from msmodel.add_info.runtime_op_info_model import RuntimeOpInfoModel, RuntimeOpInfoViewModel
from profiling_bean.db_dto.runtime_op_info_dto import RuntimeOpInfoDto

NAMESPACE = 'msmodel.add_info.runtime_op_info_model'


class TestRuntimeOpInfoModel(unittest.TestCase):

    def test_flush_for_parse_model(self):
        with mock.patch(NAMESPACE + '.RuntimeOpInfoModel.insert_data_to_db'):
            self.assertEqual(RuntimeOpInfoModel('test').flush([]), None)

    def test_get_runtime_op_info_data_for_view_model(self):
        with mock.patch('common_func.db_manager.DBManager.judge_table_exist', return_value=False):
            self.assertEqual(RuntimeOpInfoViewModel('test').get_runtime_op_info_data(), {})

        with mock.patch('common_func.db_manager.DBManager.judge_table_exist', return_value=True), \
            mock.patch('common_func.db_manager.DBManager.fetch_all_data', return_value=[RuntimeOpInfoDto()]):
            self.assertEqual(RuntimeOpInfoViewModel('test').get_runtime_op_info_data(), {(0, 0, 0): RuntimeOpInfoDto()})


if __name__ == '__main__':
    unittest.main()

#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import unittest
from unittest import mock

from constant.constant import CONFIG
from mscalculate.ge.ge_hash_calculator import GeHashCalculator
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'mscalculate.ge.ge_hash_calculator'


class TestGeHashCalculator(unittest.TestCase):
    file_list = {DataTag.GE_TASK: ['Framework.task_desc_info.0.slice_0']}

    def test_get_ge_task_data(self):
        with mock.patch('model.interface.view_model.ViewModel.check_table', return_value={0: 'test'}), \
                mock.patch('model.interface.view_model.ViewModel.get_all_data', return_value=[[1, 1, 0, 8, 1]]):
            check = GeHashCalculator(self.file_list, CONFIG)
            check._ge_data = [(1, 0, 1, 1, 8, 0)]
            check.get_ge_task_data()
            self.assertEqual(check._ge_data, [[1, 1, 0, 8, 1]])

    def test_update_data(self):
        with mock.patch(NAMESPACE + '.get_ge_hash_dict', return_value={0: 'test'}):
            check = GeHashCalculator(self.file_list, CONFIG)
            check._ge_data = [(1, 0, 1, 1, 8, 0)]
            check.update_data()
            self.assertEqual(check._ge_data, [[1, 'test', 1, 1, 8, 0]])

    def test_calculate(self):
        with mock.patch(NAMESPACE + '.GeHashCalculator.get_ge_task_data'), \
                mock.patch(NAMESPACE + '.GeHashCalculator.update_data'):
            check = GeHashCalculator(self.file_list, CONFIG)
            check.calculate()

    def test_save(self):
        with mock.patch('model.ge.ge_info_model.GeModel.init'), \
                mock.patch('model.ge.ge_info_model.GeModel.insert_data_to_db'), \
                mock.patch('model.ge.ge_info_model.GeModel.delete_table'), \
                mock.patch('model.ge.ge_info_model.GeModel.finalize'):
            check = GeHashCalculator(self.file_list, CONFIG)
            check._aic_data_list = [123]
            check.save()

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.GeHashCalculator.calculate'), \
                mock.patch(NAMESPACE + '.GeHashCalculator.save'):
            check = GeHashCalculator(self.file_list, CONFIG)
            check.ms_run()

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
from profiling_bean.db_dto.ge_task_dto import GeTaskDto
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'mscalculate.ge.ge_hash_calculator'


class TestGeHashCalculator(unittest.TestCase):
    file_list = {DataTag.GE_TASK: ['Framework.task_desc_info.0.slice_0']}

    @staticmethod
    def _construct_ge_task_dto():
        ge_task_info = GeTaskDto()
        ge_task_info.model_id = 1
        ge_task_info.op_name = 0
        ge_task_info.stream_id = 1
        ge_task_info.task_id = 1
        ge_task_info.block_dim = 1
        ge_task_info.op_state = 0
        ge_task_info.task_type = 'AI_CORE'
        ge_task_info.op_type = 0
        ge_task_info.index_id = 1
        ge_task_info.thread_id = 1000
        ge_task_info.timestamp = 123456
        ge_task_info.batch_id = 1
        return ge_task_info

    def test_get_ge_task_data(self):
        with mock.patch('msmodel.interface.view_model.ViewModel.check_table', return_value={0: 'test'}), \
                mock.patch('msmodel.interface.view_model.ViewModel.get_all_data',
                           return_value=[self._construct_ge_task_dto()]):
            check = GeHashCalculator(self.file_list, CONFIG)
            ret = check.get_ge_task_data()
            self.assertTrue(isinstance(ret[0], GeTaskDto))

    def test_update_data(self):
        with mock.patch(NAMESPACE + '.get_ge_hash_dict', return_value={0: 'test'}):
            check = GeHashCalculator(self.file_list, CONFIG)
            check.update_data([self._construct_ge_task_dto()])
            self.assertEqual(check._ge_data, [[1, 'test', 1, 1, 1, 0, 'AI_CORE', 'test', 1, 1000, 123456, 1]])

    def test_calculate(self):
        with mock.patch(NAMESPACE + '.GeHashCalculator.get_ge_task_data'), \
                mock.patch(NAMESPACE + '.GeHashCalculator.update_data'):
            check = GeHashCalculator(self.file_list, CONFIG)
            check.calculate()

    def test_save(self):
        with mock.patch('msmodel.ge.ge_info_model.GeModel.init'), \
                mock.patch('msmodel.ge.ge_info_model.GeModel.insert_data_to_db'), \
                mock.patch('msmodel.ge.ge_info_model.GeModel.delete_table'), \
                mock.patch('msmodel.ge.ge_info_model.GeModel.finalize'):
            check = GeHashCalculator(self.file_list, CONFIG)
            check._aic_data_list = [123]
            check.save()

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.GeHashCalculator.calculate'), \
                mock.patch(NAMESPACE + '.GeHashCalculator.save'):
            check = GeHashCalculator(self.file_list, CONFIG)
            check.ms_run()

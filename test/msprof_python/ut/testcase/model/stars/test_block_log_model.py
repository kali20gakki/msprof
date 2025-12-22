#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.

from unittest import mock

from common_func.db_name_constant import DBNameConstant
from model.test_dir_cr_base_model import TestDirCRBaseModel
from msmodel.stars.block_log_model import BlockLogModel
from msmodel.stars.block_log_model import BlockLogViewModel

NAMESPACE = 'msmodel.stars.block_log_model'


class TestBlockLogModel(TestDirCRBaseModel):
    def test_flush_given_blank_list_when_flush_data_then_flush_succesfully(self):
        with mock.patch(NAMESPACE + '.BlockLogModel.insert_data_to_db'):
            check = BlockLogModel('test', 'test', [])
            check.flush([])

    def test_get_summary_data(self):
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[(1, 2)]), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=1):
            check = BlockLogViewModel('test', 'test', [])
            ret = check.get_summary_data()
        self.assertEqual(ret, [(1, 2)])

    def test_get_timeline_data(self):
        with mock.patch(NAMESPACE + '.BlockLogViewModel.get_all_data', return_value=1):
            check = BlockLogViewModel('test', 'test', [])
            ret = check.get_timeline_data()
        self.assertEqual(ret, 1)

    def test_get_block_log_data_when_three_pieces_data_in_table_then_get_all_data(self):
        task_type_ai_core = "AI_CORE"
        log_data = [
            [2, 23, 1, task_type_ai_core, 1, 128, 1000, 2000, 1000],
            [2, 24, 2, task_type_ai_core, 1, 128, 3000, 4000, 1000],
            [2, 25, 3, task_type_ai_core, 1, 128, 5000, 6000, 1000],
        ]
        model = BlockLogModel(self.PROF_DEVICE_DIR, DBNameConstant.DB_SOC_LOG, [DBNameConstant.TABLE_BLOCK_LOG])
        model.init()
        model.flush(log_data)
        model.finalize()

        with BlockLogViewModel(self.PROF_DEVICE_DIR, DBNameConstant.DB_SOC_LOG,
                                [DBNameConstant.TABLE_BLOCK_LOG]) as view_model:
            view_model.init()
            device_tasks = view_model.get_block_log_data()
            self.assertEqual(len(device_tasks), 3)

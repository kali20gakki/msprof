#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
from unittest import mock

from common_func.db_name_constant import DBNameConstant
from model.test_dir_cr_base_model import TestDirCRBaseModel
from msmodel.task_time.ascend_task_model import AscendTaskModel
from msmodel.task_time.ascend_task_model import AscendTaskViewModel


NAMESPACE = "msmodel.task_time.ascend_task_model"


class TestAscendTaskModel(TestDirCRBaseModel):

    def test_get_ascend_task_data_without_unknown_should_return_all_data(self):
        data = [
            [0, 1, 27, 2, 4294967295, 0, 38140478706523, 1510560, "KERNEL_AICPU", "AI_CPU", 0],
            [0, 1, 36, 2, 47, 0, 38140480645103, 12400, "FFTS_PLUS", "AIV", 1],
        ]
        with AscendTaskModel(self.PROF_HOST_DIR, [DBNameConstant.TABLE_ASCEND_TASK]) as model:
            model.create_table()
            model.insert_data_to_db(DBNameConstant.TABLE_ASCEND_TASK, data)
            task_data = model.get_ascend_task_data_without_unknown()
            self.assertEqual(len(task_data), 2)

    def test_get_ascend_task_data_with_op_name_pattern_and_stream_id_should_return_empty_when_no_data(self):
        with mock.patch(NAMESPACE + ".DBManager.judge_table_exist", return_value=True), \
                mock.patch(NAMESPACE + ".AscendTaskViewModel.attach_to_db", return_value=True), \
                mock.patch(NAMESPACE + ".DBManager.fetch_all_data", return_value=[]):
            with AscendTaskViewModel(self.PROF_HOST_DIR, [DBNameConstant.TABLE_ASCEND_TASK]) as model:
                data = model.get_ascend_task_data_with_op_name_pattern_and_stream_id(
                    0,
                    'AICPU_KERNEL',
                    (0, )
                )
                self.assertEqual(0, len(data))


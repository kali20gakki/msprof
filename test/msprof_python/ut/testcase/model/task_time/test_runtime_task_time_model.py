#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
import os

from common_func.db_name_constant import DBNameConstant
from model.test_dir_cr_base_model import TestDirCRBaseModel
from msmodel.task_time.runtime_task_time_model import RuntimeTaskTimeModel

NAMESPACE = 'msmodel.stars.acsq_task_model'


class TestRuntimeTaskTimeModel(TestDirCRBaseModel):
    def test_get_runtime_task_data_within_time_range_by_different_time_staggered_situation(self):
        ts_data = [
            [0, 0, "api", 0, 0, 1, 23, 0, 0, 1000, 2000, 0, 0, 0],
            [0, 0, "api", 0, 0, 2, 23, 0, 0, 3000, 4000, 0, 0, 0],
            [0, 0, "api", 0, 0, 3, 23, 0, 0, 5000, 6000, 0, 0, 0],
            [0, 0, "api", 0, 0, 4, 24, 0, 0, 7000, 8000, 0, 0, 0],
        ]
        model = RuntimeTaskTimeModel(self.PROF_HOST_DIR)
        model.init()
        model.create_table()
        model.insert_data_to_db(DBNameConstant.TABLE_RUNTIME_TASK_TIME, ts_data)

        device_tasks = model.get_runtime_task_data_within_time_range(3500, 4500)
        self.assertEqual(len(device_tasks), 1)

        device_tasks = model.get_runtime_task_data_within_time_range(0, 6000)
        self.assertEqual(len(device_tasks), 3)

        device_tasks = model.get_runtime_task_data_within_time_range(0, 4999)
        self.assertEqual(len(device_tasks), 2)

        device_tasks = model.get_runtime_task_data_within_time_range(7800, 8000)
        self.assertEqual(len(device_tasks), 1)

        device_tasks = model.get_runtime_task_data_within_time_range(200, 500)
        self.assertEqual(len(device_tasks), 0)

        model.finalize()

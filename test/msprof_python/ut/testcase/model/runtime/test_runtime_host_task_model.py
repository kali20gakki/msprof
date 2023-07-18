#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
from common_func.db_name_constant import DBNameConstant
from model.test_dir_cr_base_model import TestDirCRBaseModel
from msmodel.runtime.runtime_host_task_model import RuntimeHostTaskModel


class TestRuntimeHostTaskModel(TestDirCRBaseModel):
    def test__get_host_tasks_should_return_empty_when_no_db(self):
        model = RuntimeHostTaskModel(self.PROF_DIR)
        tasks = model.get_host_tasks(True, 1, 0)
        self.assertEqual(tasks, [])

    def test__get_host_tasks_should_return_no_empty_when_not_all_in_different_scene(self):
        host_data = [
            [2, 0, 2, 15, 0, "AI_CORE", 1000],
            [2, 0, 2, 16, 0, "AI_CORE", 1001],
            [2, 0, 2, 15, 1, "AI_CORE", 1002],
            # dynamic and static mix
            [2, 1, 2, 25, 1, "AI_CORE", 1002.5],
            [3, 1, 3, 22, 0, "AI_CORE", 1003],
            [3, 1, 3, 23, 0, "AI_CORE", 1004],
            [3, 1, 3, 22, 1, "AI_CORE", 1005],
            # model unload
            [5, 0, 2, 15, 0, "AI_CORE", 1009],
        ]
        model = RuntimeHostTaskModel(self.PROF_DEVICE_DIR)
        model.init()
        model.create_table()
        model.flush(host_data, DBNameConstant.TABLE_HOST_TASK)

        mix_dynamic_and_static_host_tasks = model.get_host_tasks(False, 2, 1)
        self.assertEqual(len(mix_dynamic_and_static_host_tasks), 4)

        pure_dynamic_host_tasks = model.get_host_tasks(False, 3, 1)
        self.assertEqual(len(pure_dynamic_host_tasks), 3)

        host_tasks = model.get_host_tasks(False, 3, 2)
        self.assertEqual(len(host_tasks), 0)

        host_tasks_after_unload = model.get_host_tasks(False, 5, 2)
        self.assertEqual(len(host_tasks_after_unload), 1)
        model.finalize()

    def test__get_host_tasks_should_return_no_empty_when_all_in_different_scene(self):
        host_data = [
            [4294967295, -1, 4, 22, 0, "AI_CORE", 1006],
            [4294967295, -1, 4, 23, 0, "AI_CORE", 1007],
            [4294967295, -1, 4, 22, 1, "AI_CORE", 1008],
        ]
        model = RuntimeHostTaskModel(self.PROF_DEVICE_DIR)
        model.init()
        model.create_table()
        model.flush(host_data, DBNameConstant.TABLE_HOST_TASK)

        host_tasks = model.get_host_tasks(True, -1, -1)
        self.assertEqual(len(host_tasks), 3)

        model.finalize()

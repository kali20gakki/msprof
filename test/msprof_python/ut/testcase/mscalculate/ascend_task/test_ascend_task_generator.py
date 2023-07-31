#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from mscalculate.ascend_task.ascend_task import DeviceTask
from mscalculate.ascend_task.ascend_task import HostTask
from mscalculate.ascend_task.ascend_task import TopDownTask
from mscalculate.ascend_task.ascend_task_generator import AscendTaskGenerator

NAMESPACE = 'mscalculate.ascend_task.ascend_task_generator'


class TestAscendTaskGenerator(unittest.TestCase):
    def setUp(self) -> None:
        info_reader = InfoConfReader()
        info_reader._info_json = {'platform_version': "5"}

    def tearDown(self) -> None:
        info_reader = InfoConfReader()
        info_reader._info_json = {}

    def test__sep_device_task_by_stream_should_return_3_stream(self):
        device_tasks = [
            DeviceTask(1, 2, 0, 1000, 1000, "AI_CORE"),
            DeviceTask(1, 3, 0, 1000, 1000, "AI_CORE"),
            DeviceTask(2, 2, 0, 1000, 1000, "AI_CORE"),
        ]
        generator = AscendTaskGenerator("")
        tasks = generator._sep_task_by_stream_task(device_tasks)
        self.assertEqual(len(tasks), 3)

    def test__sep_host_task_by_stream_should_return_3_stream(self):
        host_tasks = [
            HostTask(1, 1, 1, 2, 0, "AI_CORE", 1000),
            HostTask(1, 1, 1, 3, 0, "AI_CORE", 2000),
            HostTask(1, 1, 2, 2, 0, "AI_CORE", 3000),
        ]
        generator = AscendTaskGenerator("")
        tasks = generator._sep_task_by_stream_task(host_tasks)
        self.assertEqual(len(tasks), 3)

    def test__match_host_device_task_should_return_3_matched_1_host_mismatch(self):
        device_tasks = [
            DeviceTask(1, 2, 0, 1000, 1000, "AI_CORE"),
            DeviceTask(1, 2, 0, 1000, 1000, "AI_CORE"),
            DeviceTask(1, 2, 0, 1000, 1000, "AI_CORE"),
        ]
        host_tasks = [
            HostTask(1, 1, 1, 2, 0, "AI_CORE", 1000),
            HostTask(1, 1, 1, 2, 0, "AI_CORE", 2000),
            HostTask(1, 1, 1, 2, 0, "AI_CORE", 3000),
            HostTask(1, 1, 1, 2, 0, "AI_CORE", 3000),
        ]
        generator = AscendTaskGenerator("")
        tasks = generator._match_host_device_task(host_tasks, device_tasks)
        self.assertEqual(len(tasks[0]), 3)
        self.assertEqual(len(tasks[1]), 1)
        self.assertEqual(len(tasks[2]), 0)

    def test__match_host_device_task_should_return_3_matched_1_device_mismatch(self):
        device_tasks = [
            DeviceTask(1, 2, 0, 1000, 1000, "AI_CORE"),
            DeviceTask(1, 2, 0, 1000, 1000, "AI_CORE"),
            DeviceTask(1, 2, 0, 1000, 1000, "AI_CORE"),
            DeviceTask(1, 2, 0, 1000, 1000, "AI_CORE"),
        ]
        host_tasks = [
            HostTask(1, 1, 1, 2, 0, "AI_CORE", 1000),
            HostTask(1, 1, 1, 2, 0, "AI_CORE", 2000),
            HostTask(1, 1, 1, 2, 0, "AI_CORE", 3000),
        ]
        generator = AscendTaskGenerator("")
        tasks = generator._match_host_device_task(host_tasks, device_tasks)
        self.assertEqual(len(tasks[0]), 3)
        self.assertEqual(len(tasks[1]), 0)
        self.assertEqual(len(tasks[2]), 1)

    def test__insert_sub_tasks_of_acsq_task(self):
        ascend_tasks = [
            TopDownTask(1, 0, 1, 2, 4294967295, 0, 1000, 2000, "AI_CORE", "AI_CORE"),
            TopDownTask(1, 0, 1, 3, 4294967295, 0, 3800, 5200, "AI_CORE", "AI_CORE"),
            TopDownTask(1, 0, 1, 4, 4294967295, 0, 5300, 1000, "AI_CORE", "AI_CORE"),
        ]

        generator = AscendTaskGenerator("")
        ret_sub_tasks = [
            DeviceTask(1, 2, 0, 1000, 1000, "MIX_AIC"),
            DeviceTask(1, 2, 1, 1200, 1000, "MIX_AIC"),
            DeviceTask(1, 2, 2, 1500, 1000, "MIX_AIC"),
            DeviceTask(1, 2, 3, 1800, 1000, "MIX_AIC"),
            DeviceTask(1, 3, 0, 4000, 1000, "MIX_AIC"),
            DeviceTask(1, 3, 1, 4100, 1000, "MIX_AIC"),
        ]
        with mock.patch(NAMESPACE + ".DeviceTaskCollector.get_sub_tasks_by_time_range",
                        return_value=ret_sub_tasks):
            tasks = generator._insert_sub_tasks_of_acsq_task(ascend_tasks)
            self.assertEqual(len(tasks), 9)

    def test__generate_top_down_tasks_when_pure_static_graph_scene(self):
        """
        model_id: int
        index_id: int
        stream_id: int
        task_id: int
        batch_id: int
        task_type: str
        host_timestamp: int
        """
        host_tasks = [
            HostTask(1, 0, 1, 2, 0, "AI_CORE", 1000),
            HostTask(1, 0, 1, 3, 0, "AI_CORE", 2000),
            HostTask(1, 0, 1, 4, 0, "AI_CORE", 3000),
        ]

        """
        stream_id: int
        task_id: int
        subtask_id: int
        start_time: int
        duration: int
        task_type: str
        """
        device_tasks = [
            DeviceTask(1, 2, 4294967295, 1000, 1000, "AI_CORE"),
            DeviceTask(1, 3, 4294967295, 3000, 1000, "AI_CORE"),
            DeviceTask(2, 2, 4294967295, 5000, 1000, "AI_CORE"),
            DeviceTask(1, 4, 4294967295, 7000, 1000, "AI_CORE"),
            DeviceTask(1, 2, 4294967295, 9000, 1000, "AI_CORE"),
        ]

        generator = AscendTaskGenerator("")
        matched, all_ = generator._generate_top_down_tasks(host_tasks, device_tasks)
        self.assertEqual(len(matched), 4)
        self.assertEqual(len(all_), 5)

    def test__generate_top_down_tasks_when_static_graph_mix_with_dynamic_graph_scene(self):
        host_tasks = [
            HostTask(1, 0, 1, 2, 0, "AI_CORE", 1000),
            HostTask(1, 0, 1, 3, 0, "AI_CORE", 2000),
            HostTask(1, 0, 1, 4, 0, "AI_CORE", 3000),
            HostTask(1, 1, 1, 6, 0, "AI_CORE", 5000),
            HostTask(1, 1, 1, 7, 0, "AI_CORE", 6000),
            HostTask(1, 1, 1, 8, 0, "AI_CORE", 7000),
            HostTask(1, 1, 1, 65535, 0, "AI_CORE", 8000),
            HostTask(1, 1, 1, 6, 1, "AI_CORE", 9000),
        ]

        device_tasks = [
            DeviceTask(1, 2, 4294967295, 1000, 1000, "AI_CORE"),
            DeviceTask(1, 3, 4294967295, 3000, 1000, "AI_CORE"),
            DeviceTask(2, 2, 4294967295, 5000, 1000, "AI_CORE"),
            DeviceTask(1, 4, 4294967295, 7000, 1000, "AI_CORE"),
            DeviceTask(1, 2, 4294967295, 9000, 1000, "AI_CORE"),
            DeviceTask(1, 6, 4294967295, 11000, 1000, "AI_CORE"),
            DeviceTask(1, 7, 4294967295, 13000, 1000, "AI_CORE"),
            DeviceTask(1, 8, 4294967295, 15000, 1000, "AI_CORE"),
            DeviceTask(1, 9, 4294967295, 17000, 1000, "AI_CORE"),
            DeviceTask(1, 6, 4294967295, 19000, 1000, "AI_CORE"),
        ]

        generator = AscendTaskGenerator("")
        matched, all_ = generator._generate_top_down_tasks(host_tasks, device_tasks)
        self.assertEqual(len(matched), 8)
        self.assertEqual(len(all_), 11)

    def test__generate_top_down_tasks_operate_scene(self):
        host_tasks = [
            HostTask(4294967295, -1, 1, 2, 0, "AI_CORE", 1000),
            HostTask(4294967295, -1, 1, 3, 0, "AI_CORE", 2000),
            HostTask(4294967295, -1, 1, 4, 0, "AI_CORE", 3000),
            HostTask(4294967295, -1, 1, 6, 0, "AI_CORE", 5000),
            HostTask(4294967295, -1, 1, 7, 0, "AI_CORE", 6000),
            HostTask(4294967295, -1, 1, 8, 0, "AI_CORE", 7000),
            HostTask(4294967295, -1, 1, 65535, 0, "AI_CORE", 8000),
            HostTask(4294967295, -1, 1, 6, 1, "AI_CORE", 9000),
        ]

        device_tasks = [
            DeviceTask(1, 2, 4294967295, 1000, 1000, "AI_CORE"),
            DeviceTask(1, 3, 4294967295, 3000, 1000, "AI_CORE"),
            DeviceTask(2, 2, 4294967295, 5000, 1000, "AI_CORE"),
            DeviceTask(1, 4, 4294967295, 7000, 1000, "AI_CORE"),
            DeviceTask(1, 2, 4294967295, 9000, 1000, "AI_CORE"),
            DeviceTask(1, 6, 4294967295, 11000, 1000, "AI_CORE"),
            DeviceTask(1, 7, 4294967295, 13000, 1000, "AI_CORE"),
            DeviceTask(1, 8, 4294967295, 15000, 1000, "AI_CORE"),
            DeviceTask(1, 9, 4294967295, 17000, 1000, "AI_CORE"),
            DeviceTask(1, 6, 4294967295, 19000, 1000, "AI_CORE"),
        ]

        generator = AscendTaskGenerator("")
        matched, all_ = generator._generate_top_down_tasks(host_tasks, device_tasks)
        self.assertEqual(len(matched), 7)
        self.assertEqual(len(all_), 11)

    def test__get_all_ascend_tasks(self):
        generator = AscendTaskGenerator("")
        with mock.patch(NAMESPACE + ".HostTaskCollector.get_host_tasks", return_value=[]), \
                mock.patch(NAMESPACE + ".DeviceTaskCollector.get_all_device_tasks", return_value=[]):
            tasks = generator._get_all_ascend_tasks()
            self.assertEqual(tasks, ([], []))

    def test__get_ascend_tasks_within_iter(self):
        generator = AscendTaskGenerator("")
        with mock.patch(NAMESPACE + ".HostTaskCollector.get_host_tasks_by_model_and_iter", return_value=[]), \
                mock.patch(NAMESPACE + ".DeviceTaskCollector.get_device_tasks_by_model_and_iter", return_value=[]):
            tasks = generator._get_ascend_tasks_within_iter(10, 1)
            self.assertEqual(tasks, ([], []))

    def test_run_graph_scene(self):
        generator = AscendTaskGenerator("")
        with mock.patch(NAMESPACE + ".AscendTaskGenerator._get_ascend_tasks_within_iter",
                        return_value=([], [TopDownTask(1, 1, 1, 1, 1, 1, 1000, 1000, 'AI_CORE', 'AI_CORE')])):
            tasks = generator.run(10, 1, 4)
            self.assertEqual(len(tasks), 4)

    def test_run_operate_scene(self):
        generator = AscendTaskGenerator("")
        with mock.patch(NAMESPACE + ".AscendTaskGenerator._get_all_ascend_tasks",
                        return_value=([], [TopDownTask(1, 1, 1, 1, 1, 1, 1000, 1000, 'AI_CORE', 'AI_CORE')])):
            tasks = generator.run(4294967295, 1, 4)
            self.assertEqual(len(tasks), 1)

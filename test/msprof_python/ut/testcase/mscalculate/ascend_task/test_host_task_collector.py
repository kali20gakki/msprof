#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
import os
import unittest
from unittest import mock

from mscalculate.ascend_task.ascend_task import HostTask
from mscalculate.ascend_task.host_task_collector import HostTaskCollector

file_list = {}
NAMESPACE = 'mscalculate.ascend_task.host_task_collector'


class TestHostTaskCollector(unittest.TestCase):
    DIR_PATH = os.path.join(os.path.dirname(__file__), "DT_HostTaskCollector")
    PROF_DIR = os.path.join(DIR_PATH, 'PROF1')
    collector = HostTaskCollector(PROF_DIR)

    def test__get_host_tasks_should_return_empty_when_no_table(self):
        self.assertEqual([], self.collector._get_host_tasks(True, 1, 1))

    def test__get_host_tasks_should_return_not_empty(self):
        with mock.patch(NAMESPACE + ".DBManager.check_tables_in_db", return_value=True), \
                mock.patch(NAMESPACE + ".RuntimeHostTaskModel.get_host_tasks",
                           return_value=[[1, 2, 3, 4, 5, "", 1000]]):
            self.assertEqual(len(self.collector._get_host_tasks(True, 1, 1)), 1)

    def test__filter_dev_visible_host_tasks_should_return_not_none(self):
        all_host_tasks = [
            HostTask(1, 2, 3, 4, 5, "", 1000),
            HostTask(1, 2, 3, 4, 6, "", 2000)
        ]
        dev_visible_ids = [(3, 4, 5)]
        visible_tasks = self.collector._filter_dev_visible_host_tasks(all_host_tasks, dev_visible_ids)
        self.assertEqual(len(visible_tasks), 1)

    def test__check_host_tasks_exists(self):
        with mock.patch(NAMESPACE + ".DBManager.check_tables_in_db", return_value=True):
            self.assertTrue(self.collector._check_host_tasks_exists())
        with mock.patch(NAMESPACE + ".DBManager.check_tables_in_db", return_value=False):
            self.assertFalse(self.collector._check_host_tasks_exists())

    def test_get_host_tasks_by_model_and_iter(self):
        with mock.patch(NAMESPACE + ".HostTaskCollector._check_host_tasks_exists", return_value=False):
            visible_host_tasks = self.collector.get_host_tasks_by_model_and_iter(1, 1)
            self.assertEqual(len(visible_host_tasks), 0)

        with mock.patch(NAMESPACE + ".HostTaskCollector._check_host_tasks_exists", return_value=True), \
                mock.patch(NAMESPACE + ".HostTaskCollector._get_host_tasks", return_value=[]):
            visible_host_tasks = self.collector.get_host_tasks_by_model_and_iter(1, 1)
            self.assertEqual(len(visible_host_tasks), 0)

        all_host_tasks = [
            HostTask(1, 2, 3, 4, 0, "", 1000),
            HostTask(1, 2, 4, 7, 1, "", 1000),
        ]
        with mock.patch(NAMESPACE + ".HostTaskCollector._check_host_tasks_exists", return_value=True), \
                mock.patch(NAMESPACE + ".HostTaskCollector._get_host_tasks", return_value=all_host_tasks):
            visible_host_tasks = self.collector.get_host_tasks_by_model_and_iter(1, 2)
            self.assertEqual(len(visible_host_tasks), 2)

    def test_get_host_tasks(self):
        with mock.patch(NAMESPACE + ".HostTaskCollector._check_host_tasks_exists", return_value=False):
            visible_host_tasks = self.collector.get_host_tasks()
            self.assertEqual(len(visible_host_tasks), 0)

        with mock.patch(NAMESPACE + ".HostTaskCollector._check_host_tasks_exists", return_value=True), \
                mock.patch(NAMESPACE + ".HostTaskCollector._get_host_tasks", return_value=[]):
            visible_host_tasks = self.collector.get_host_tasks()
            self.assertEqual(len(visible_host_tasks), 0)

        all_host_tasks = [
            HostTask(1, 2, 3, 4, 0, "", 1000),
            HostTask(1, 2, 4, 7, 1, "", 1000),
        ]
        with mock.patch(NAMESPACE + ".HostTaskCollector._check_host_tasks_exists", return_value=True), \
                mock.patch(NAMESPACE + ".HostTaskCollector._get_host_tasks", return_value=all_host_tasks):
            visible_host_tasks = self.collector.get_host_tasks()
            self.assertEqual(len(visible_host_tasks), 2)

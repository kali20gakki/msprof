#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2022. All rights reserved.
"""
import unittest
from unittest import mock

from common_func.msvp_constant import MsvpConstant
from constant.info_json_construct import InfoJson
from constant.info_json_construct import InfoJsonReaderManager
from viewer.runtime.runtime_api_viewer import RuntimeApiViewer

NAMESPACE = 'viewer.runtime.runtime_api_viewer'


class TestRuntimeApiViewer(unittest.TestCase):
    def test_get_summary_data_should_return_empty_when_model_init_fail(self):
        config = {"headers": ["Name", "Stream ID", "Time(%)", "Time(ns)", "Calls",
                              "Avg(ns)", "Min(ns)", "Max(ns)", "Process ID", "Thread ID"]}
        params = {
            "project": "test_runtime_api_view",
            "model_id": 1,
            "iter_id": 1
        }
        check = RuntimeApiViewer(config, params)
        ret = check.get_summary_data()
        self.assertEqual(MsvpConstant.MSVP_EMPTY_DATA, ret)

    def test_get_summary_data_should_return_empty_when_model_init_ok(self):
        config = {"headers": ["Name", "Stream ID", "Time(%)", "Time(ns)", "Calls",
                              "Avg(ns)", "Min(ns)", "Max(ns)", "Process ID", "Thread ID"]}
        params = {
            "project": "test_runtime_api_view",
            "model_id": 1,
            "iter_id": 1
        }
        with mock.patch(NAMESPACE + '.RuntimeApiViewModel.init', return_value=True), \
                mock.patch(NAMESPACE + ".RuntimeApiViewModel.get_summary_data", return_value=[]):
            check = RuntimeApiViewer(config, params)
            ret = check.get_summary_data()
            self.assertEqual(MsvpConstant.MSVP_EMPTY_DATA, ret)

    def test_get_timeline_data_should_return_empty_when_model_init_fail(self):
        config = {"headers": ["Name", "Stream ID", "Time(%)", "Time(ns)", "Calls",
                              "Avg(ns)", "Min(ns)", "Max(ns)", "Process ID", "Thread ID"]}
        params = {
            "project": "test_runtime_api_view",
            "model_id": 1,
            "iter_id": 1
        }
        check = RuntimeApiViewer(config, params)
        ret = check.get_timeline_data()
        self.assertEqual('{"status": 1, '
                         '"info": "Failed to connect acl_module.db"}', ret)

    def test_get_timeline_data_should_return_empty_when_model_init_ok(self):
        config = {"headers": ["Name", "Stream ID", "Time(%)", "Time(ns)", "Calls",
                              "Avg(ns)", "Min(ns)", "Max(ns)", "Process ID", "Thread ID"]}
        params = {
            "project": "test_runtime_api_view",
            "model_id": 1,
            "iter_id": 1
        }
        with mock.patch(NAMESPACE + '.RuntimeApiViewModel.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.RuntimeApiViewModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + ".RuntimeApiViewModel.get_timeline_data", return_value=[]):
            check = RuntimeApiViewer(config, params)
            ret = check.get_timeline_data()
            self.assertEqual('{"status": 2, '
                             '"info": "Unable to get runtime api data."}', ret)

    def test_get_timeline_data_should_return_empty_when_data_exist(self):
        config = {"headers": ["Name", "Stream ID", "Time(%)", "Time(ns)", "Calls",
                              "Avg(ns)", "Min(ns)", "Max(ns)", "Process ID", "Thread ID"]}
        params = {
            "project": "test_runtime_api_view",
            "model_id": 1,
            "iter_id": 1
        }
        with mock.patch(NAMESPACE + '.RuntimeApiViewModel.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.RuntimeApiViewModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + ".RuntimeApiViewModel.get_timeline_data",
                           return_value=[(1, 2, 3, 4, 5, 6, 7, 8, 9, 10)]):
            InfoJsonReaderManager(InfoJson(pid=100)).process()
            check = RuntimeApiViewer(config, params)
            ret = check.get_timeline_data()
            self.assertEqual('[{"name": "process_name", '
                             '"pid": 100, "tid": 0, '
                             '"args": {"name": "Runtime"}, "ph": "M"}, '
                             '{"name": "thread_name", '
                             '"pid": 100, "tid": 4, '
                             '"args": {"name": "Thread 4"}, "ph": "M"}, '
                             '{"name": "thread_sort_index", '
                             '"pid": 100, "tid": 4, '
                             '"args": {"sort_index": 4}, "ph": "M"}, '
                             '{"name": "1", "pid": 100, "tid": 4, '
                             '"ts": 0.002, "dur": 0.003, '
                             '"args": {"Thread Id": 4}, "ph": "X"}]', ret)

#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2023. All rights reserved.
"""
import unittest
from unittest import mock

from constant.info_json_construct import InfoJson
from constant.info_json_construct import InfoJsonReaderManager
from viewer.api_viewer import ApiViewer

NAMESPACE = 'viewer.api_viewer'


class TestApiViewer(unittest.TestCase):

    def test_get_timeline_data_should_return_empty_when_model_init_fail(self):
        config = {"headers": []}
        params = {
            "project": "test_api_view",
            "model_id": 1,
            "iter_id": 1
        }
        check = ApiViewer(config, params)
        ret = check.get_timeline_data()
        self.assertEqual('{"status": 1, '
                         '"info": "Failed to connect api_event.db"}', ret)

    def test_get_timeline_data_should_return_empty_when_model_init_ok(self):
        config = {"headers": []}
        params = {
            "project": "test_api_view",
            "model_id": 1,
            "iter_id": 1
        }
        with mock.patch(NAMESPACE + '.ApiDataViewModel.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.ApiDataViewModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + ".ApiDataViewModel.get_timeline_data", return_value=[]):
            check = ApiViewer(config, params)
            ret = check.get_timeline_data()
            self.assertEqual('{"status": 2, '
                             '"info": "Unable to get api data."}', ret)

    def test_get_timeline_data_should_return_empty_when_data_exist(self):
        config = {"headers": []}
        params = {
            "project": "test_api_view",
            "model_id": 1,
            "iter_id": 1
        }
        with mock.patch(NAMESPACE + '.ApiDataViewModel.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.ApiDataViewModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + ".ApiDataViewModel.get_timeline_data",
                           return_value=[(1, 2, 3, 4, 5, 6, 7, 8, 9, 10)]):
            InfoJsonReaderManager(InfoJson(pid=100)).process()
            check = ApiViewer(config, params)
            ret = check.get_timeline_data()
            self.assertEqual('[{"name": "process_name", '
                             '"pid": 100, "tid": 0, '
                             '"args": {"name": "Api"}, "ph": "M"}, '
                             '{"name": "thread_name", '
                             '"pid": 100, "tid": 4, '
                             '"args": {"name": "Thread 4"}, "ph": "M"}, '
                             '{"name": "thread_sort_index", "pid": 100,'
                             ' "tid": 4, "args": {"sort_index": 4}, "ph": "M"}, '
                             '{"name": "1", "pid": 100, "tid": 4, '
                             '"ts": 0.002, "dur": 0.003, "args": '
                             '{"Thread Id": 4, "Mode": 1, "level": 5, "id": 6, '
                             '"item_id": 7}, "ph": "X"}]', ret)

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
from profiling_bean.db_dto.api_data_dto import ApiDataDto
from viewer.event_viewer import EventViewer

NAMESPACE = 'viewer.event_viewer'


class TestEventViewer(unittest.TestCase):



    def test_get_timeline_data_should_return_empty_when_model_init_fail(self):
        config = {"headers": []}
        params = {
            "project": "test_event_view",
            "model_id": 1,
            "iter_id": 1
        }
        check = EventViewer(config, params)
        ret = check.get_timeline_data()
        self.assertEqual('{"status": 1, '
                         '"info": "Failed to connect api_event.db"}', ret)

    def test_get_timeline_data_should_return_empty_when_model_init_ok(self):
        config = {"headers": []}
        params = {
            "project": "test_event_view",
            "model_id": 1,
            "iter_id": 1
        }
        with mock.patch(NAMESPACE + '.EventDataViewModel.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.EventDataViewModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + ".EventDataViewModel.get_timeline_data", return_value=[]):
            check = EventViewer(config, params)
            ret = check.get_timeline_data()
            self.assertEqual('{"status": 2, '
                             '"info": "Unable to get event data."}', ret)

    def test_get_timeline_data_should_return_empty_when_data_exist(self):
        config = {"headers": []}
        params = {
            "project": "test_event_view",
            "model_id": 1,
            "iter_id": 1
        }
        matched_event_dto = ApiDataDto()
        matched_event_dto.struct_type = "1"
        matched_event_dto.id = "2"
        matched_event_dto.start = 3
        matched_event_dto.end = 4
        matched_event_dto.thread_id = 5
        matched_event_dto.item_id = 6
        matched_event_dto.level = "7"
        matched_event_dto.request_id = 8
        with mock.patch(NAMESPACE + '.EventDataViewModel.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.EventDataViewModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + ".EventDataViewModel.get_timeline_data", return_value=[matched_event_dto]):
            InfoJsonReaderManager(InfoJson(pid=100)).process()
            check = EventViewer(config, params)
            ret = check.get_timeline_data()
            self.assertEqual('[{"name": "process_name", '
                             '"pid": 100, "tid": 0, '
                             '"args": {"name": "Event"}, "ph": "M"}, '
                             '{"name": "thread_name", '
                             '"pid": 100, "tid": 5, '
                             '"args": {"name": "Thread 5"}, "ph": "M"}, '
                             '{"name": "thread_sort_index", '
                             '"pid": 100, "tid": 5, '
                             '"args": {"sort_index": 5}, "ph": "M"}, '
                             '{"name": "1", "pid": 100, "tid": 5, '
                             '"ts": 0.003, "dur": 0.001, '
                             '"args": {"Thread Id": 5, "level": "7", '
                             '"id": "2", "item_id": 6, "request_id": 8}, '
                             '"ph": "X"}]', ret)
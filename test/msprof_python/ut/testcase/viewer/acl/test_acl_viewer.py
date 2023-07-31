#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2022. All rights reserved.
"""
import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from common_func.msvp_constant import MsvpConstant
from viewer.acl.acl_viewer import AclViewer

NAMESPACE = 'viewer.acl.acl_viewer'


class TestAclViewer(unittest.TestCase):
    def test_get_summary_data_should_return_empty_when_model_init_fail(self):
        config = {"headers": ["Name", "Type", "Start Time",
                              "Duration(us)", "Process ID", "Thread ID"]}
        params = {
            "project": "test_acl_view",
            "model_id": 1,
            "iter_id": 1
        }
        check = AclViewer(config, params)
        ret = check.get_summary_data()
        self.assertEqual(MsvpConstant.MSVP_EMPTY_DATA, ret)

    def test_get_summary_data_should_return_success_when_model_init_ok(self):
        config = {"headers": ["Name", "Type", "Start Time",
                              "Duration(us)", "Process ID", "Thread ID"]}
        params = {
            "project": "test_acl_view",
            "model_id": 1,
            "iter_id": 1
        }
        InfoConfReader()._host_freq = None
        InfoConfReader()._info_json = {'CPU': [{'Frequency': "1000"}]}
        with mock.patch(NAMESPACE + '.AclModel.init', return_value=True), \
                mock.patch(NAMESPACE + '.AclModel.get_summary_data', return_value=[(1, 2, 3, 4, 5, 6)]):
            check = AclViewer(config, params)
            ret = check.get_summary_data()
            self.assertEqual((['Name', 'Type', 'Start Time', 'Duration(us)', 'Process ID', 'Thread ID'],
                              [(1, 2, 3.0, 0.004, 5, 6)],
                              1), ret)

    def test_get_acl_statistic_data_should_return_empty_when_db_check_fail(self):
        config = {"headers": ["Name", "Type", "Start Time",
                              "Duration(us)", "Process ID", "Thread ID"]}
        params = {
            "project": "test_acl_view",
            "model_id": 1,
            "iter_id": 1
        }
        check = AclViewer(config, params)
        ret = check.get_acl_statistic_data()
        self.assertEqual(MsvpConstant.MSVP_EMPTY_DATA, ret)

    def test_get_acl_statistic_data_should_return_empty_when_db_check_ok(self):
        config = {"headers": ["Name", "Type", "Start Time",
                              "Duration(us)", "Process ID", "Thread ID"]}
        params = {
            "project": "test_acl_view",
            "model_id": 1,
            "iter_id": 1
        }
        with mock.patch(NAMESPACE + '.AclModel.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.AclModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + '.AclModel.get_acl_statistic_data', return_value=[]):
            check = AclViewer(config, params)
            ret = check.get_acl_statistic_data()
            self.assertEqual(MsvpConstant.MSVP_EMPTY_DATA, ret)

    def test_get_timeline_data_should_return_empty_when_db_check_fail(self):
        config = {"headers": ["Name", "Type", "Start Time",
                              "Duration(us)", "Process ID", "Thread ID"]}
        params = {
            "project": "test_acl_view",
            "model_id": 1,
            "iter_id": 1
        }
        check = AclViewer(config, params)
        ret = check.get_timeline_data()
        self.assertEqual('{"status": 1, '
                         '"info": "No acl data found, maybe the switch of acl is not on."}', ret)

    def test_get_timeline_data_should_return_empty_when_db_check_ok(self):
        config = {"headers": ["Name", "Type", "Start Time",
                              "Duration(us)", "Process ID", "Thread ID"]}
        params = {
            "project": "test_acl_view",
            "model_id": 1,
            "iter_id": 1
        }
        with mock.patch(NAMESPACE + '.AclModel.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.AclModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + '.AclModel.get_timeline_data', return_value=[]):
            check = AclViewer(config, params)
            ret = check.get_timeline_data()
            self.assertEqual('{"status": 1, '
                             '"info": "Failed to connect acl_module.db."}', ret)

    def test_get_timeline_data_should_return_empty_when_data_exist(self):
        config = {"headers": ["Name", "Type", "Start Time",
                              "Duration(us)", "Process ID", "Thread ID"]}
        params = {
            "project": "test_acl_view",
            "model_id": 1,
            "iter_id": 1
        }
        with mock.patch(NAMESPACE + '.AclModel.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.AclModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + '.AclModel.get_timeline_data', return_value=[(1, 2, 3, 4, 5, 6)]):
            check = AclViewer(config, params)
            ret = check.get_timeline_data()
            self.assertEqual('[{"name": "process_name", '
                             '"pid": 4, "tid": 0, '
                             '"args": {"name": "AscendCL"}, "ph": "M"}, '
                             '{"name": "thread_name", '
                             '"pid": 4, "tid": 5, '
                             '"args": {"name": "Thread 5"}, "ph": "M"}, '
                             '{"name": "thread_sort_index", '
                             '"pid": 4, "tid": 5, '
                             '"args": {"sort_index": 5}, "ph": "M"}, '
                             '{"name": 1, "pid": 4, "tid": 5, '
                             '"ts": 0.002, "dur": 0.003, '
                             '"args": {"Mode": 6, "Process Id": 4, "Thread Id": 5}, "ph": "X"}]', ret)

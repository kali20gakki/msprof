#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2022. All rights reserved.
"""
import unittest
from unittest import mock

from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from sqlite.db_manager import DBOpen
from viewer.thread_group_viewer import ThreadGroupViewer

NAMESPACE = 'viewer.thread_group_viewer'


class TestThreadGroupViewer(unittest.TestCase):

    def test_get_timeline_data(self):
        with mock.patch(NAMESPACE + '.ThreadGroupViewer._get_acl_api_data', return_value=[]), \
             mock.patch(NAMESPACE + '.ThreadGroupViewer._get_ge_time_data', return_value=[]), \
             mock.patch(NAMESPACE + '.ThreadGroupViewer._get_ge_op_execute_data', return_value=[]), \
             mock.patch(NAMESPACE + '.ThreadGroupViewer._get_runtime_api_data', return_value=[]):
            param = {"project": "home"}
            InfoConfReader()._info_json = {'pid': 0}
            check = ThreadGroupViewer(None, param)
            ret = check.get_timeline_data()
            self.assertEqual('[]', ret)

    def test_get_acl_api_data(self):
        create_sql = "CREATE TABLE IF NOT EXISTS " + DBNameConstant.TABLE_ACL_DATA + \
                     " (api_name, api_type, start_time, end_time, process_id, thread_id, device_id)"
        acl_data = [("aclmdlLoadFromFileWithMem", "ACL_MODEL", 123456, 123459, 0, 4745, 0)]
        with DBOpen(DBNameConstant.DB_ACL_MODULE) as db_open:
            db_open.create_table(create_sql)
            db_open.insert_data(DBNameConstant.TABLE_ACL_DATA, acl_data)
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(db_open.db_conn, db_open.db_curs)):
                param = {"project": "home"}
                InfoConfReader()._info_json = {'pid': 0}
                check = ThreadGroupViewer(None, param)
                ret = check._get_acl_api_data()
                self.assertEqual("ACL@aclmdlLoadFromFileWithMem", ret[0].get('name'))

    def test_get_ge_time_data(self):
        create_sql = "CREATE TABLE IF NOT EXISTS " + DBNameConstant.TABLE_GE_MODEL_TIME + \
                     " (model_name, model_id, request_id, thread_id, input_start, input_end, " \
                     "infer_start, infer_end, output_start, output_end)"
        ge_data = [("resnet50", 1, 0, 6914, 123456, 123459, 123546, 123549, 123656, 123659)]
        with DBOpen(DBNameConstant.DB_GE_MODEL_TIME) as db_open:
            db_open.create_table(create_sql)
            db_open.insert_data(DBNameConstant.TABLE_GE_MODEL_TIME, ge_data)
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(db_open.db_conn, db_open.db_curs)):
                param = {"project": "home"}
                InfoConfReader()._info_json = {'pid': 0}
                check = ThreadGroupViewer(None, param)
                ret = check._get_ge_time_data()
                self.assertEqual(3, len(ret))

    def test_get_ge_op_execute_data(self):
        create_sql = "CREATE TABLE IF NOT EXISTS " + DBNameConstant.TABLE_GE_HOST + \
                     " (thread_id, op_type, event_type, start_time, end_time)"
        ge_op_execute_data = [(5138, 0, 5806034091292471464, 123456, 123459)]
        with DBOpen(DBNameConstant.DB_GE_HOST_INFO) as db_open:
            db_open.create_table(create_sql)
            db_open.insert_data(DBNameConstant.TABLE_GE_HOST, ge_op_execute_data)
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(db_open.db_conn, db_open.db_curs)), \
                    mock.patch(NAMESPACE + '.get_ge_hash_dict', return_value={}):
                param = {"project": "home"}
                InfoConfReader()._info_json = {'pid': 0}
                check = ThreadGroupViewer(None, param)
                ret = check._get_ge_op_execute_data()
                self.assertEqual(1, len(ret))

    def test_get_runtime_api_data(self):
        create_sql = "CREATE TABLE IF NOT EXISTS " + DBNameConstant.TABLE_API_CALL + \
                     " (entry_time, exit_time, api, retcode, thread, stream_id," \
                     "tasknum, task_id, batch_id, data_size, memcpy_direction)"
        api_call_data = [(123456, 123459, "SetDevice", 0, 5458, 65535, 0, "", "", 0, 0)]
        with DBOpen(DBNameConstant.DB_RUNTIME) as db_open:
            db_open.create_table(create_sql)
            db_open.insert_data(DBNameConstant.TABLE_API_CALL, api_call_data)
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(db_open.db_conn, db_open.db_curs)):
                param = {"project": "home"}
                InfoConfReader()._info_json = {'pid': 0}
                check = ThreadGroupViewer(None, param)
                ret = check._get_runtime_api_data()
                self.assertEqual(1, len(ret))

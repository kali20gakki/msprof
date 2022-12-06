#!/usr/bin/python3
# coding=utf-8
import json
import sqlite3
import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from viewer.get_hccl_export_data import HCCLExport

NAMESPACE = 'viewer.get_hccl_export_data'
PARAMS = {'data_type': 'hccl', 'project': '',
          'device_id': '1', 'job_id': 'job_default', 'export_type': 'timeline',
          'iter_id': 1, 'export_format': None, 'model_id': -1}


class TestHCCLExport(unittest.TestCase):

    def test_get_hccl_timeline_data(self):
        InfoConfReader()._info_json = {"pid": 1}
        with mock.patch(NAMESPACE + '.HCCLExport.get_hccl_sql', return_value=" "), \
                mock.patch(NAMESPACE + '.HCCLExport._get_hccl_sql_data', return_value=[]):
            res = HCCLExport(PARAMS).get_hccl_timeline_data()
        self.assertEqual(res, json.dumps([]))

    def test_format_hccl_data(self):
        hccl_data = [
            ('Notify Wait', 2, 162191740576.88998, 25.76, 'NULL', 27, 2, 'Notify Wait', 'LOCAL', None, 0, 4294967385,
             'allreduce')]
        InfoConfReader()._info_json = {"pid": 1}
        hccl = HCCLExport(PARAMS)
        hccl._format_hccl_data(hccl_data)
        self.assertEqual(len(hccl.result), 4)

    def test_get_hccl_sql(self):
        sql = "select name,plane_id,timestamp,duration,bandwidth,stream_id," \
              "task_id, task_type,transport_type,size,stage,step from HCCLAllReduce where timestamp>=0.0 " \
              "and timestamp<10.0"
        InfoConfReader()._info_json = {"pid": 1}
        with mock.patch(NAMESPACE + '.Utils.get_scene', return_value="step_info"), \
                mock.patch(NAMESPACE + '.MsprofIteration.get_iteration_time', return_value=[[0, 10000]]):
            res = HCCLExport(PARAMS).get_hccl_sql()

    def test_get_hccl_sql_data_1(self):
        InfoConfReader()._info_json = {"pid": 1}
        sql = ""
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=[None, None]), \
                mock.patch(NAMESPACE + '.Utils.get_scene', return_value="train"), \
                mock.patch(NAMESPACE + '.MsprofIteration.get_iteration_time', return_value=[]):
            res = HCCLExport(PARAMS)._get_hccl_sql_data(sql)
        self.assertEqual(res, [])

    def test_get_hccl_sql_data_2(self):
        sql = ""
        InfoConfReader()._info_json = {"pid": 1}
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=[None, None]), \
                mock.patch(NAMESPACE + '.HCCLExport._check_hccl_table', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]):
            res = HCCLExport(PARAMS)._get_hccl_sql_data(sql)
        self.assertEqual(res, [])

    def test_get_hccl_sql_data_3(self):
        sql = ""
        InfoConfReader()._info_json = {"pid": 1}
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', side_effect=sqlite3.Error):
            res = HCCLExport(PARAMS)._get_hccl_sql_data(sql)
        self.assertEqual(res, [])

#!/usr/bin/python3
# coding=utf-8
import json
import sqlite3
import unittest
from collections import namedtuple
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from constant.constant import ITER_RANGE
from profiling_bean.db_dto.hccl_dto import HcclDto
from sqlite.db_manager import DBManager
from viewer.get_hccl_export_data import HCCLExport

NAMESPACE = 'viewer.get_hccl_export_data'
PARAMS = {'data_type': 'hccl', 'project': '',
          'device_id': '1', 'job_id': 'job_default', 'export_type': 'timeline',
          'iter_id': ITER_RANGE, 'export_format': None, 'model_id': -1}


class TestHCCLExport(unittest.TestCase):

    def test_get_hccl_timeline_data(self):
        InfoConfReader()._info_json = {"pid": 1}
        db_manager = DBManager()
        table_name = 'HCCLAllReduce'
        create_sql = "CREATE TABLE IF NOT EXISTS {0} (model_id int, index_id int, _iteration int)".format(table_name)

        data = ((1, 1, 1),)
        insert_sql = "insert into {0} values ({1})".format(table_name, "?," * (len(data[0]) - 1) + "?")
        conn, curs = db_manager.create_table('hccl.db', create_sql, insert_sql, data)
        with mock.patch('msmodel.hccl.hccl_model.HcclViewModel.get_hccl_op_data', return_value=[]):
            res = HCCLExport(PARAMS).get_hccl_timeline_data()
        self.assertEqual(json.loads(res).get('status'), 1)
        with mock.patch('msmodel.hccl.hccl_model.HcclViewModel.get_all_data', return_value=[]),\
                mock.patch('msmodel.interface.view_model.DBManager.create_connect_db', return_value=(conn, curs)),\
                mock.patch('msmodel.hccl.hccl_model.HcclViewModel.check_table', return_value=True):
            res = HCCLExport(PARAMS).get_hccl_timeline_data()
        self.assertEqual(json.loads(res).get('status'), 2)
        conn, curs = db_manager.create_table('hccl.db')
        with mock.patch(NAMESPACE + '.HCCLExport._format_hccl_data'),\
                mock.patch('msmodel.interface.view_model.DBManager.create_connect_db', return_value=(conn, curs)),\
                mock.patch('msmodel.hccl.hccl_model.HcclViewModel.check_table', return_value=True):
            res = HCCLExport(PARAMS).get_hccl_timeline_data()
        self.assertEqual(json.loads(res), [])
        conn, curs = db_manager.create_table('hccl.db')
        db_manager.destroy((conn, curs))

    def test_format_hccl_data(self):
        hccl_data = namedtuple('hccl', ['plane_id', 'task_id', 'stream_id', 'hccl_name', 'step', 'stage', 'duration',
                                        'timestamp', 'args'])
        hccl_data = [hccl_data(1, 2, 3, 0, 0, 0, 0, 0, {}), hccl_data(2, 3, 4, 0, 0, 0, 0, 0, {})]

        model_data = namedtuple('model', ['op_name', 'timestamp', 'args', 'duration'])
        model_data = [model_data('test_1', 0, [1, 2, 3], 0)]
        InfoConfReader()._info_json = {"pid": 1}
        with mock.patch('msmodel.hccl.hccl_model.HcclViewModel.get_hccl_op_data', return_value=model_data):
            hccl = HCCLExport(PARAMS)
            hccl._format_hccl_data(hccl_data)
            self.assertEqual(len(hccl.result), 10)


if __name__ == '__main__':
    unittest.main()

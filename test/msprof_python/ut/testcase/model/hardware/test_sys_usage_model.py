import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from msmodel.hardware.sys_usage_model import SysUsageModel

NAMESPACE = 'msmodel.hardware.sys_usage_model'


class TestSysUsageModel(unittest.TestCase):

    def test_create_table(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False), \
                mock.patch(NAMESPACE + '.DBManager.sql_create_general_table'), \
                mock.patch(NAMESPACE + '.DBManager.execute_sql'):
            check = SysUsageModel('test', 'sys_usage_0.db', ['SysCpuUsage'])
            check.create_table()

    def test_flush(self):
        with mock.patch(NAMESPACE + '.SysUsageModel.insert_data_to_db'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = SysUsageModel('test', 'tscpu_0.db', ['TsOriginalData'])
            check.flush({'sys_data_list': [1], 'pid_data_list': [2]})

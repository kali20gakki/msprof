import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from model.hardware.sys_mem_model import SysMemModel

NAMESPACE = 'model.hardware.sys_mem_model'


class TestSysMemModel(unittest.TestCase):

    def test_flush(self):
        with mock.patch(NAMESPACE + '.SysMemModel.insert_data_to_db'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = SysMemModel('test', 'tscpu_0.db', ['TsOriginalData'])
            check.flush({'sys_mem_data': [1], 'pid_mem_data': [2]})

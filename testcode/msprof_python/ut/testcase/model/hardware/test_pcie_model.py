import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from model.hardware.pcie_model import PcieModel

NAMESPACE = 'model.hardware.pcie_model'


class TestPcieModel(unittest.TestCase):

    def test_create_table(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False), \
                mock.patch(NAMESPACE + '.DBManager.sql_create_general_table'), \
                mock.patch(NAMESPACE + '.DBManager.execute_sql'):
            check = PcieModel('test', 'pcie.db', ['PcieOriginalData'])
            check.create_table()

    def test_flush(self):
        with mock.patch(NAMESPACE + '.PcieModel.insert_data_to_db'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = PcieModel('test', 'pcie.db', ['PcieOriginalData'])
            check.flush([])

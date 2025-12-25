import unittest
from unittest import mock

from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from msmodel.hardware.ub_model import UBModel

NAMESPACE = 'msmodel.hardware.ub_model'


class TestUBModel(unittest.TestCase):
    TABLE_LIST = [
        DBNameConstant.TABLE_UB_BW
    ]
    TEMP_DIR = 'test'

    def test_create_table_should_return_true_when_create_table(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.sql_create_general_table'), \
                mock.patch(NAMESPACE + '.UBModel.drop_table'), \
                mock.patch(NAMESPACE + '.DBManager.execute_sql'):
            check = UBModel(TestUBModel.TEMP_DIR, DBNameConstant.DB_UB, TestUBModel.TABLE_LIST)
            check.create_table()

    def test_flush_should_return_true_when_insert_empty(self):
        with mock.patch(NAMESPACE + '.UBModel.insert_data_to_db'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = UBModel(TestUBModel.TEMP_DIR, DBNameConstant.DB_UB, TestUBModel.TABLE_LIST)
            check.flush([])

import unittest
from unittest import mock

from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.msprof_exception import ProfException
from msmodel.hardware.qos_model import QosModel
from sqlite.db_manager import DBManager

NAMESPACE = 'msmodel.hardware.qos_model'


class TestQosModel(unittest.TestCase):
    TABLE_LIST = [
        DBNameConstant.TABLE_QOS_INFO,
        DBNameConstant.TABLE_QOS_ORIGIN
    ]
    TEMP_DIR = 'test'

    def test_create_table(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.sql_create_general_table'), \
                mock.patch(NAMESPACE + '.QosModel.drop_table'), \
                mock.patch(NAMESPACE + '.DBManager.execute_sql'):
            check = QosModel(TestQosModel.TEMP_DIR, DBNameConstant.DB_QOS, TestQosModel.TABLE_LIST)
            check.create_table()

    def test_flush(self):
        with mock.patch(NAMESPACE + '.QosModel.insert_data_to_db'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = QosModel(TestQosModel.TEMP_DIR, DBNameConstant.DB_QOS, TestQosModel.TABLE_LIST)
            check.flush(DBNameConstant.TABLE_QOS_ORIGIN, [])

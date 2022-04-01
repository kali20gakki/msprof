import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from common_func.msprof_exception import ProfException
from model.hardware.hbm_model import HbmModel
from sqlite.db_manager import DBManager

NAMESPACE = 'model.hardware.hbm_model'


class TestHbmModel(unittest.TestCase):

    def test_create_table(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.sql_create_general_table'), \
                mock.patch(NAMESPACE + '.HbmModel.drop_tab'), \
                mock.patch(NAMESPACE + '.DBManager.execute_sql'):
            check = HbmModel('test', 'hbm.db', ['HBMbwData', 'HBMOriginalData'])
            check.create_table()

    def test_drop_tab(self):
        InfoConfReader()._info_json = {'devices': '0'}
        db_manager = DBManager()
        res = db_manager.create_table('hbm.db')
        check = HbmModel('test', 'hbm.db', ['HBMbwData', 'HBMOriginalData'])
        check.conn, check.cur = res[0], res[1]
        check.drop_tab()
        db_manager.destroy(res)
        with mock.patch(NAMESPACE + '.logging.error'):
            check = HbmModel('test', 'hbm.db', ['HBMbwData', 'HBMOriginalData'])
            check.conn, check.cur = res[0], res[1]
            check.drop_tab()

    def test_flush(self):
        with mock.patch(NAMESPACE + '.HbmModel.insert_data_to_db'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = HbmModel('test', 'hbm.db', ['HBMbwData', 'HBMOriginalData'])
            check.flush([])

    def test_insert_bw_data(self):
        original_sql = "CREATE TABLE IF NOT EXISTS HBMOriginalData(device_id INT,replayid INT," \
                       "timestamp REAL,counts INT,event_type TEXT,hbmid INT)"
        insert_sql = "insert into {} values (?,?,?,?,?,?)".format('HBMOriginalData')
        data = ((5, 0, 2, 3, 4, 5), (5, 0, 0, 0, 0, 0),
                (5, 0, 1, 1, 1, 1), (5, 0, 3, 3, 3, 3))
        db_manager = DBManager()
        res = db_manager.create_table('hbm.db', original_sql, insert_sql, data)
        check = HbmModel('test', 'hbm.db', ['HBMbwData', 'HBMOriginalData'])
        check.conn, check.cur = res[0], res[1]
        check.insert_bw_data(["read", "write"])
        res[1].execute('CREATE TABLE IF NOT EXISTS HBMbwData(device_id INT,timestamp REAL,'
                       'bandwidth REAL,hbmid INT,event_type TEXT)')
        check = HbmModel('test', 'hbm.db', ['HBMbwData', 'HBMOriginalData'])
        check.conn, check.cur = res[0], res[1]
        check.insert_bw_data(["read"])
        with mock.patch(NAMESPACE + '.logging.error'):
            check = HbmModel('test', 'hbm.db', ['HBMbwData', 'HBMOriginalData'])
            check.conn, check.cur = res[0], res[1]
            try:
                check.insert_bw_data(["read", "write", "write"])
            except ProfException as err:
                self.assertEqual(err.code, ProfException.PROF_SYSTEM_EXIT)
        res[1].execute("drop table HBMOriginalData")
        res[1].execute("drop table HBMbwData")
        db_manager.destroy(res)

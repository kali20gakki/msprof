import sqlite3
import unittest
from unittest import mock
from common_func.msvp_constant import MsvpConstant
from viewer.ts_cpu_report import TsCpuReport

from sqlite.db_manager import DBManager

NAMESPACE = 'viewer.ts_cpu_report'


class TesTsCpuReport(unittest.TestCase):
    def test_get_output_top_function_1(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', side_effect=sqlite3.DatabaseError):
            check = TsCpuReport()
            res = check.get_output_top_function("tscpu_0.db", '')
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(None, None)), \
             mock.patch(NAMESPACE + '.pre_check_sample', return_value=None), \
             mock.patch(NAMESPACE + '.error'):
            check = TsCpuReport()
            res = check.get_output_top_function("tscpu_0.db", '')
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(None, None)), \
             mock.patch(NAMESPACE + '.pre_check_sample', return_value=True), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            check = TsCpuReport()
            res = check.get_output_top_function("tscpu_0.db", '')
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

    def test_get_output_top_function__2(self):
        create_sql = "CREATE TABLE IF NOT EXISTS TsOriginalData" \
                     " (function, count, event)"
        data = (("", 1, "0x11"),)
        db_manager = DBManager()
        insert_sql = db_manager.insert_sql("TsOriginalData", data)

        test_sql = db_manager.create_table("tscpu_0.db", create_sql, insert_sql, data)

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=test_sql), \
             mock.patch(NAMESPACE + '.pre_check_sample', return_value=True), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            check = TsCpuReport()
            res = check.get_output_top_function("tscpu_0.db", '')

        test_sql = db_manager.connect_db("tscpu_0.db")
        (test_sql[1]).execute("DELETE from TsOriginalData")
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=test_sql), \
             mock.patch(NAMESPACE + '.pre_check_sample', return_value=True), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            check = TsCpuReport()
            res_1 = check.get_output_top_function("tscpu_0.db", '')
        test_sql = db_manager.connect_db("tscpu_0.db")
        (test_sql[1]).execute("drop Table TsOriginalData")
        db_manager.destroy(test_sql)
        self.assertEqual(len(res[0]), 3)
        self.assertEqual(res_1, MsvpConstant.MSVP_EMPTY_DATA)


if __name__ == '__main__':
    unittest.main()

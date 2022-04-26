import sqlite3
import unittest
from unittest import mock
from common_func.msvp_constant import MsvpConstant
from sqlite.db_manager import DBManager
from viewer.cpu_data_report import get_aictrl_pmu_events, get_ts_pmu_events, get_cpu_hot_function

NAMESPACE = 'viewer.cpu_data_report'
headers = ['Event', 'Name', 'Count']
sample_config = {"ts_cpu_profiling_events": "0x0,0x1,0x2"}


class TestCPUDataReport(unittest.TestCase):
    def test_get_aictrl_pmu_events_1(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', side_effect=sqlite3.Error):
            res = get_aictrl_pmu_events('', 'aicpu_0.db', 'OriginalData', headers)
            self.assertEqual(MsvpConstant.MSVP_EMPTY_DATA, res)

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            res = get_aictrl_pmu_events('', 'aicpu_0.db', 'OriginalData', headers)
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

    def test_get_aictrl_pmu_events_2(self):
        create_sql = "CREATE TABLE IF NOT EXISTS OriginalData (common, pid, tid, core, timestamp," \
                     " pmucount, pmuevent, ip, function, offset, module, replayid)"
        data = (("swapper", 0, 0, 7, 1467.43561, 8017770, "r11", "ffff000008087e80", "unknown",
                 "unknown", "[unknown]", 0),)
        insert_sql = "insert into {0} values ({value})".format(
            "OriginalData", value="?," * (len(data[0]) - 1) + "?")
        db_manager = DBManager()
        test_sql = db_manager.create_table("aicpu_0.db", create_sql, insert_sql, data)

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=test_sql), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            res = get_aictrl_pmu_events('', 'aicpu_0.db', 'OriginalData', headers)
        self.assertEqual(res[2], 1)

        test_sql = db_manager.connect_db('aicpu_0.db')
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=test_sql), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
             mock.patch(NAMESPACE + '.read_cpu_cfg', return_value=None):
            res = get_aictrl_pmu_events('', 'aicpu_0.db', 'OriginalData', headers)
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)
        test_sql = db_manager.connect_db('aicpu_0.db')
        (test_sql[1]).execute("drop Table OriginalData")
        db_manager.destroy(test_sql)

    def test_get_ts_pmu_events_1(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
             mock.patch(NAMESPACE + '.pre_check_sample', return_value=sample_config), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', side_effect=sqlite3.Error):
            res = get_ts_pmu_events('', 'tscpu_0.db', 'OriginalData', headers)
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(None, None)):
            res = get_ts_pmu_events('', 'tscpu_0.db', 'OriginalData', headers)
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            res = get_ts_pmu_events('', 'tscpu_0.db', 'OriginalData', headers)
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

    def test_get_ts_pmu_events_2(self):
        create_sql = "CREATE TABLE IF NOT EXISTS OriginalData (count, event)"
        data = ((1, "0x0"),)
        insert_sql = "insert into {0} values ({value})".format(
            "OriginalData", value="?," * (len(data[0]) - 1) + "?")
        db_manager = DBManager()
        test_sql = db_manager.create_table("tscpu_0.db", create_sql, insert_sql, data)

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=test_sql), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
             mock.patch(NAMESPACE + '.pre_check_sample', return_value=sample_config):
            res = get_ts_pmu_events('', 'tscpu_0.db', 'OriginalData', headers)
        self.assertEqual(res[2], 1)

        test_sql = db_manager.connect_db('tscpu_0.db')
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=test_sql), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
             mock.patch(NAMESPACE + '.pre_check_sample', return_value=sample_config), \
             mock.patch(NAMESPACE + '.read_cpu_cfg', return_value=None):
            res = get_ts_pmu_events('', 'tscpu_0.db', 'OriginalData', headers)
        self.assertEqual(res[2], 1)
        test_sql = db_manager.connect_db('tscpu_0.db')
        (test_sql[1]).execute("drop Table OriginalData")
        db_manager.destroy(test_sql)

    def test_get_cpu_hot_function_1(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', side_effect=sqlite3.Error):
            res = get_cpu_hot_function('', 'aicpu_0.db', 'OriginalData', headers)
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            res = get_cpu_hot_function('', 'aicpu_0.db', 'OriginalData', headers)
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

    def test_get_cpu_hot_function_2(self):
        db_manager = DBManager()
        create_sql = "CREATE TABLE IF NOT EXISTS OriginalData (common, pid, tid, core, timestamp," \
                     " pmucount, cycles, ip, func, offset, module, r11)"
        create_accu_sql = "CREATE TABLE IF NOT EXISTS OriginalData (common, pid, tid, core, timestamp," \
                          " pmucount, cycles, ip, func, offset, module, r11)"
        data = (("swapper", 0, 0, 7, 1467.43561, 8017770, "r11", "ffff000008087e80", "unknown",
                 "unknown", "[unknown]", 3),)
        insert_sql = "insert into {0} values ({value})".format(
            "OriginalData", value="?," * (len(data[0]) - 1) + "?")
        insert_accu_sql = "insert into {0} values ({value})".format(
            "OriginalData", value="?," * (len(data[0]) - 1) + "?")
        db_manager.create_table("aicpu_0.db", create_accu_sql, insert_accu_sql, data)
        test_sql = db_manager.create_table("aicpu_0.db", create_sql, insert_sql, data)

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=test_sql), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            res = get_cpu_hot_function('', 'aicpu_0.db', 'OriginalData', headers)
        self.assertEqual(res[2], 1)

        test_sql = db_manager.connect_db('aicpu_0.db')
        (test_sql[1]).execute("delete from OriginalData")
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=test_sql), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            res = get_cpu_hot_function('', 'aicpu_0.db', 'OriginalData', headers)
        self.assertEqual(res[2], 0)
        test_sql = db_manager.connect_db('aicpu_0.db')
        (test_sql[1]).execute("drop Table OriginalData")
        db_manager.destroy(test_sql)


if __name__ == '__main__':
    unittest.main()

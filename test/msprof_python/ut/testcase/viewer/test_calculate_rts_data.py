import sqlite3
import unittest
from unittest import mock
import logging
import pytest
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from sqlite.db_manager import DBManager
from viewer.calculate_rts_data import calculate_task_schedule_data, \
    multi_calculate_task_cost_time, calculate_timeline_task_time, handle_task_time, check_aicore_events, \
    create_ai_event_tables, insert_event_value, insert_metric_value, get_limit_and_offset, \
    get_metrics_from_sample_config, insert_metric_summary_table, calculate_timeline_task_time, \
    _get_pmu_data, _get_stream_and_task_id, insert_event_value
from common_func.msprof_exception import ProfException

NAMESPACE = 'viewer.calculate_rts_data'
sample_config = {'ai_core_profiling_events': '0x8,0x9',
                 "ai_core_metrics": "PipeUtilization"}
info_json = {"DeviceInfo": [{"id": 0, "env_type": 3, "ctrl_cpu_id": "ARMv8_Cortex_A55", "ctrl_cpu_core_num": 4,
                             "ctrl_cpu_endian_little": 1, "ts_cpu_core_num": 1, "ai_cpu_core_num": 4,
                             "ai_core_num": 2, "ai_cpu_core_id": 4,
                             "ai_core_id": 0, "aicpu_occupy_bitmap": 240, "ctrl_cpu": "0,1,2,3", "ai_cpu": "4,5,6,7",
                             "aiv_num": 0, "hwts_frequency": "680", "aic_frequency": "680", "aiv_frequency": "1000"}]}


class TestCalculateRts(unittest.TestCase):
    def test_calculate_task_schedule_data_1(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=None), \
                mock.patch(NAMESPACE + '.logging.info'):
            res = calculate_task_schedule_data('', 0)
        self.assertEqual(res, [])

    def test_calculate_task_schedule_data_2(self):
        create_sql = "CREATE TABLE IF NOT EXISTS TaskTime(replayid, device_id, api, apirowid, tasktype, " \
                     "task_id, stream_id, waittime, pendingtime, running, complete, index_id, model_id, batch_id)"
        data = ((0, 0, 4, 430, 4, 2, 5, 0, 0, 118562436328.0, 118562438671.0, 1, 1, 0),)
        insert_sql = "insert into {0} values ({value})".format(
            "TaskTime", value="?," * (len(data[0]) - 1) + "?")
        db_manager = DBManager()
        test_sql = db_manager.create_table(DBNameConstant.DB_RUNTIME, create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            res = calculate_task_schedule_data(test_sql[1], 0)
        self.assertEqual(len(res), 1)
        (test_sql[1]).execute("drop Table TaskTime")
        db_manager.destroy(test_sql)

    def test_handle_task_time_1(self):
        data = [(1, 0, '', '', 1, 2, 3, 5, 3)]
        list_data = [-1, -1, -1]
        res = handle_task_time(0, [], data, list_data)
        self.assertEqual(res[0], -1)

    def test_handle_task_time_2(self):
        data = [(1, 0, '', '', 1, 2, 3, 5, 2)]
        list_data = [-1, -1, -1]
        res = handle_task_time(0, [], data, list_data)
        self.assertEqual(res[0], -1)

    def test_handle_task_time_3(self):
        data = [(1, 0, '', '', 1, 2, 3, 5, 1)]
        list_data = [-1, -1, -1]
        res = handle_task_time(0, [], data, list_data)
        self.assertEqual(res[0], -1)

    def test_handle_task_time_4(self):
        data = [(1, 0, '', '', 1, 2, 3, 5, 10)]
        list_data = [-1, -1, -1]
        with mock.patch(NAMESPACE + '.logging.error'):
            res = handle_task_time(0, [], data, list_data)
        self.assertEqual(res[0], -1)

    def test_check_aicore_events_1(self):
        with pytest.raises(SystemExit) as error:
            check_aicore_events(None)
        self.assertEqual(error.value.args[0], 1)

    def test_check_aicore_events_2(self):
        events = ["0x8", "0xa", "0x9", "0xb", "0xc", "0xd", "0x54", "0x55", 0x1]
        check_aicore_events(events)

    def test_create_ai_event_tables(self):
        create_sql = "CREATE TABLE IF NOT EXISTS EventCounter(replayid, tasktype, task_id, stream_id, overflow, " \
                     "overflowcycle, timestamp, event1, event2, event3, event4, event5, event6, event7, event8, " \
                     "task_cyc, block, thread, device_id, mode)"
        data = ((0, 0, 3, 5, 0, 0, 101612566070, 26050, 0, 526, 0, 14002, 26675, 224, 12, 58006, 2, 0, 0, 1),)
        insert_sql = "insert into {0} values ({value})".format(
            "EventCounter", value="?," * (len(data[0]) - 1) + "?")
        db_manager = DBManager()
        test_sql = db_manager.create_table(DBNameConstant.DB_RUNTIME, create_sql, insert_sql, data)
        create_ai_event_tables(sample_config, test_sql[1], 0)
        (test_sql[1]).execute("drop Table EventCounter")
        (test_sql[1]).execute("drop Table EventCount")
        (test_sql[1]).execute("drop Table task_cyc")
        (test_sql[1]).execute("drop Table r8")
        (test_sql[1]).execute("drop Table r9")
        (test_sql[0]).commit()
        db_manager.destroy(test_sql)

        InfoConfReader()._info_json = info_json
        conn = mock.Mock()
        curs = mock.Mock()
        curs.description = [['device_id'], ['core_num'], ['other']]
        insert_event_value(curs, conn, 0)

    def test_insert_metric_value(self):
        metrics = ["a", "b"]
        metrics_1 = []
        db_manager = DBManager()
        test_sql = db_manager.create_table(DBNameConstant.DB_RUNTIME)
        res = insert_metric_value(test_sql[0], metrics, "ts")
        self.assertEqual(res, True)

        res = insert_metric_value(test_sql[0], metrics, "ts")
        self.assertEqual(res, True)

        res = insert_metric_value(test_sql[0], metrics_1, "ts")
        self.assertEqual(res, False)

        (test_sql[1]).execute("drop Table ts")
        db_manager.destroy(test_sql)

    def test_get_limit_and_offset_1(self):
        with mock.patch(NAMESPACE + "._query_limit_and_offset", side_effect=sqlite3.OperationalError), \
                mock.patch(NAMESPACE + '.logging.error'):
            res = get_limit_and_offset('', 1, 0)
        self.assertEqual(res, [])

        with mock.patch(NAMESPACE + ".PathManager.get_db_path", return_value=""), \
                mock.patch(NAMESPACE + ".DBManager.judge_table_exist", return_value=""), \
                mock.patch(NAMESPACE + ".DBManager.check_connect_db_path", return_value=(None, None)):
            res = get_limit_and_offset('', 1, 0)
        self.assertEqual(res, [])

        with mock.patch(NAMESPACE + ".PathManager.get_db_path", return_value=""), \
                mock.patch(NAMESPACE + ".DBManager.judge_table_exist", return_value=True), \
                mock.patch(NAMESPACE + ".DBManager.check_connect_db_path", return_value=(True, True)), \
                mock.patch(NAMESPACE + ".DBManager.destroy_db_connect"), \
                mock.patch(NAMESPACE + ".DBManager.get_table_headers", return_value=["ai_core"]):
            res = get_limit_and_offset('', 1, 0)
        self.assertEqual(res, [])

    def test_get_limit_and_offset_2(self):
        create_sql = "CREATE TABLE IF NOT EXISTS " + DBNameConstant.TABLE_STEP_TRACE_DATA +\
                     "(index_id, model_id, step_start, step_end, iter_id, ai_core_num)"
        data = ((1, 1, 118562263836, 118571743672, 1, 60),)
        insert_sql = "insert into {0} values ({value})".format(
            DBNameConstant.TABLE_STEP_TRACE_DATA, value="?," * (len(data[0]) - 1) + "?")
        db_manager = DBManager()
        test_sql = db_manager.create_table(DBNameConstant.DB_STEP_TRACE, create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + ".PathManager.get_db_path", return_value=""), \
                mock.patch(NAMESPACE + ".DBManager.judge_table_exist", return_value=True), \
                mock.patch(NAMESPACE + ".DBManager.check_connect_db_path", return_value=test_sql), \
                mock.patch(NAMESPACE + ".DBManager.destroy_db_connect"), \
                mock.patch(NAMESPACE + ".DBManager.get_table_headers", return_value=["ai_core_num"]):
            res = get_limit_and_offset('', 1, 1)
            res_1 = get_limit_and_offset('', 1, 0)
        (test_sql[1]).execute("drop Table step_trace_data")
        db_manager.destroy(test_sql)
        self.assertEqual(res[1], 0)
        self.assertEqual(res_1, [])

    def test_get_metrics_from_sample_config_1(self):
        with mock.patch(NAMESPACE + ".PathManager.get_sample_json_path", return_value=True), \
                mock.patch(NAMESPACE + ".CalculateAiCoreData.add_fops_header"), \
                mock.patch(NAMESPACE + ".generate_config", return_value=sample_config):
            res_1 = get_metrics_from_sample_config('')
        self.assertEqual(len(res_1), 9)

    def test_get_metrics_from_sample_config_2(self):
        sample_config["ai_core_metrics"] = "a"
        with mock.patch(NAMESPACE + ".PathManager.get_sample_json_path", return_value=True), \
                mock.patch(NAMESPACE + ".generate_config", return_value=sample_config):
            res = get_metrics_from_sample_config('')
        self.assertEqual(res, [])
        sample_config["ai_core_metrics"] = "PipeUtilization"

    def test_insert_metric_summary_table_1(self):
        freq = 680000000.0
        metrics = ['total_time(ms)', 'total_cycles', 'vec_ratio', 'mac_ratio', 'scalar_ratio', 'mte1_ratio',
                   'mte2_ratio', 'mte3_ratio', 'icache_miss_rate']
        create_sql = "CREATE TABLE IF NOT EXISTS EventCount " \
                     "(r8, ra, r9, rb, rc, rd, r54, r55, task_cyc, task_id, stream_id, block_num, core_num, device_id)"
        data = ((26050, 0, 526, 0, 14002, 26675, 224, 12, 58006, 3, 5, 2, 2, 0),)
        insert_sql = "insert into {0} values ({value})".format(
            "EventCount", value="?," * (len(data[0]) - 1) + "?")
        db_manager = DBManager()
        test_sql = db_manager.create_table(DBNameConstant.DB_RUNTIME, create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + ".PathManager.get_db_path", return_value=""), \
                mock.patch(NAMESPACE + ".DBManager.judge_table_exist", return_value=True), \
                mock.patch(NAMESPACE + ".DBManager.check_connect_db_path", return_value=test_sql), \
                mock.patch(NAMESPACE + ".DBManager.destroy_db_connect"), \
                mock.patch(NAMESPACE + ".get_metrics_from_sample_config", return_value=metrics):
            insert_metric_summary_table('', freq, index_id=1, model_id=1, have_step_info=False)
            insert_metric_summary_table('', freq, index_id=1, model_id=1, have_step_info=True)
        (test_sql[1]).execute("drop Table EventCount")
        (test_sql[1]).execute("drop Table MetricSummary")
        test_sql[0].commit()
        db_manager.destroy(test_sql)

    def test_insert_metric_summary_table_2(self):
        db_manager = DBManager()
        test_sql = db_manager.create_table("runtime.db")
        with mock.patch(NAMESPACE + ".PathManager.get_db_path", return_value=""), \
                mock.patch(NAMESPACE + ".DBManager.judge_table_exist", return_value=True), \
                mock.patch(NAMESPACE + ".DBManager.check_connect_db_path", return_value=test_sql), \
                mock.patch(NAMESPACE + ".DBManager.destroy_db_connect"), \
                mock.patch(NAMESPACE + ".DBManager.drop_table"), \
                mock.patch(NAMESPACE + ".get_metrics_from_sample_config", return_value=[]), \
                mock.patch(NAMESPACE + ".insert_metric_value", return_value=True):
            insert_metric_summary_table('', 6800000, index_id=1, model_id=1, have_step_info=False)

        with mock.patch(NAMESPACE + ".PathManager.get_db_path", return_value=""), \
                mock.patch(NAMESPACE + ".DBManager.judge_table_exist", return_value=False), \
                mock.patch(NAMESPACE + ".DBManager.check_connect_db_path", return_value=test_sql), \
                mock.patch(NAMESPACE + ".DBManager.destroy_db_connect"):
            insert_metric_summary_table('', 6800000, index_id=1, model_id=1, have_step_info=False)

        with mock.patch(NAMESPACE + ".PathManager.get_db_path", return_value=""), \
                mock.patch(NAMESPACE + ".DBManager.judge_table_exist", return_value=False), \
                mock.patch(NAMESPACE + ".DBManager.check_connect_db_path", return_value=test_sql), \
                mock.patch(NAMESPACE + ".DBManager.destroy_db_connect"):
            insert_metric_summary_table('', 6800000, index_id=1, model_id=1, have_step_info=False)

    def test_calculate_timeline_task_time(self):
        timeline_data = [[0]*10] * 10
        with mock.patch(NAMESPACE + ".LoadInfoManager.load_info"):
            calculate_timeline_task_time(timeline_data, {}, 0, "")

    def test_get_pmu_data(self):
        curs = mock.Mock()
        _get_pmu_data([], ["test"], 0, curs)

    def test_get_stream_and_task_id(self):
        curs = mock.Mock()
        _get_stream_and_task_id([], ["test"], 0, curs)

    def test_insert_event_value(self):
        curs = mock.Mock()
        conn = mock.Mock()
        with mock.patch(NAMESPACE + ".insert_event_value"), \
            mock.patch(NAMESPACE + ".DBManager.get_table_headers", return_value=["r1", "r2", "device_id"]), \
            mock.patch(NAMESPACE + "._get_block_core_device_data"):
            insert_event_value(curs, conn, 0)


if __name__ == '__main__':
    unittest.main()

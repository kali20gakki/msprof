import os
import sqlite3
import unittest
from unittest import mock
import pytest
from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.memcpy_constant import MemoryCopyConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msvp_constant import MsvpConstant
from sqlite.db_manager import DBManager
from viewer.runtime_report import get_task_scheduler_data, add_ts_opname, \
    get_task_based_core_data, get_output_tasktype, _get_output_event_counter, get_opname, \
    add_op_total, cal_metrics, add_memcpy_data
from msmodel.runtime.runtime_api_model import get_output_apicall
from msmodel.runtime.runtime_api_model import get_runtime_api_data

NAMESPACE = 'viewer.runtime_report'
configs = {"headers": "Dvpp Id,Engine Type,Engine ID,All Time(us),All Frame,All Utilization(%)",
           "columns": "duration,bandwidth,rxBandwidth,rxPacket,rxErrorRate,rxDroppedRate,txBandwidth,txPacket,"
                      "txErrorRate,txDroppedRate,funcId"}
params = {'data_type': '',
          'project': '', 'device_id': "0",
          'job_id': 'job_default',
          'export_type': 'summary', 'iter_id': 1,
          'export_format': 'csv', 'model_id': 1}


class TestRuntimeReport(unittest.TestCase):
    def test_get_runtime_api_data_1(self):
        db_manager = DBManager()
        test_sql = db_manager.create_table("runtime.db")
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(None, None)), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            res = get_runtime_api_data('', "ApiCall", configs)
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

    def test_get_runtime_api_data_2(self):
        create_sql = "CREATE TABLE IF NOT EXISTS ApiCall" \
                     " (replayid, entry_time, exit_time, api, retcode, thread, device_id, " \
                     "stream_id, tasknum, task_id, detail, mode)"
        data = ((0, 117960819118, 117962052296, 25, 0, 2246, 0, 65535, 2, "0,0", None, 0),
                (0, 117962212631, 117962485483, 6, 0, 2246, 0, 4, 1, 0, None, 0))
        db_manager = DBManager()
        insert_sql = db_manager.insert_sql("ApiCall", data)

        test_sql = db_manager.create_table("runtime.db", create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=test_sql), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            InfoConfReader()._info_json = {'pid': 1}
            res = get_runtime_api_data('', "ApiCall", configs)
        test_sql = db_manager.connect_db("runtime.db")
        (test_sql[1]).execute("drop Table ApiCall")
        db_manager.destroy(test_sql)
        self.assertEqual(res[2], 2)

    def test_get_task_scheduler_data_1(self):
        db_manager = DBManager()
        test_sql = db_manager.create_table("runtime.db")
        configs = {"headers": "Time(%),Time(us),Count,Avg(us),Min(us),Max(us),"
                              "Waiting(us),Running(us),Pending(us),Type,API,Task ID,Op Name,Stream ID"}
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(None, None)), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            res = get_task_scheduler_data('', "ReportTask", configs, params)
        self.assertEqual(res, ("Time(%),Time(us),Count,Avg(us),Min(us),Max(us),"
                              "Waiting(us),Running(us),Pending(us),Type,API,Task ID,Op Name,Stream ID",
                               [],
                               0))

    def test_get_task_scheduler_data_2(self):
        create_sql = "CREATE TABLE IF NOT EXISTS ReportTask" \
                     " (timeratio REAL,time REAL,count INTEGER,avg REAL,min REAL,max REAL,waiting REAL,running REAL," \
                     "pending REAL,type TEXT,api TEXT,task_id INTEGER,stream_id INTEGER,device_id)"
        data = ((0.1, 1, 2, 1, 1, 5, 3, 1, 0.5, "aicore", "a", 0, 0, 0),)
        db_manager = DBManager()
        insert_sql = db_manager.insert_sql("ReportTask", data)

        configs = {"headers": "Time(%),Time(us),Count,Avg(us),Min(us),Max(us),"
                              "Waiting(us),Running(us),Pending(us),Type,API,Task ID,Op Name,Stream ID"}

        test_sql = db_manager.create_table("runtime.db", create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=test_sql), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch(NAMESPACE + '.add_memcpy_data', return_value=data):
            res = get_task_scheduler_data('', "ReportTask", configs, params)
        test_sql = db_manager.connect_db("runtime.db")
        (test_sql[1]).execute("drop Table ReportTask")
        db_manager.destroy(test_sql)
        self.assertEqual(res[2], 1)

    def test_add_memcpy_data(self):
        data = [(50, 10000, 1, 10000, 10000,
                 10000, 116.770796875, 47177.968796875, 0.0, 'model execute task', '', 2, 2),
                (50, 10000, 1, 10000, 10000,
                 10000, 0.0, 1596.19790625, 0.0, 'kernel AI core task', '', 3, 5)]
        memcpy_summary = [(0, 20000, 1, 20000, 20000, 20000, 100, 200, MemoryCopyConstant.DEFAULT_VIEWER_VALUE,
                           MemoryCopyConstant.TYPE, StrConstant.AYNC_MEMCPY, 1,
                           MemoryCopyConstant.DEFAULT_VIEWER_VALUE, 1)]
        expect_res = [(25.0, 10000, 1, 10000, 10000,
                 10000, 116.770796875, 47177.968796875, 0.0, 'model execute task', '', 2, 2),
                (25.0, 10000, 1, 10000, 10000,
                 10000, 0.0, 1596.19790625, 0.0, 'kernel AI core task', '', 3, 5),
                      (50.0, 20000, 1, 20000, 20000, 20000, 100, 200, MemoryCopyConstant.DEFAULT_VIEWER_VALUE,
                       MemoryCopyConstant.TYPE, StrConstant.AYNC_MEMCPY, 1,
                       MemoryCopyConstant.DEFAULT_VIEWER_VALUE, 1)
                      ]

        with mock.patch('viewer.memory_copy.memory_copy_viewer.'
                        'MemoryCopyViewer.get_memory_copy_chip0_summary', return_value=memcpy_summary):
            res = add_memcpy_data("./", data)
        self.assertEqual(expect_res, res)

    def test_add_ts_opname(self):
        db_manager = DBManager()
        test_sql = db_manager.create_table(DBNameConstant.DB_GE_INFO)
        task_data = [[1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1]]
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=test_sql), \
                mock.patch(NAMESPACE + '.get_opname', return_value="test"):
            res = add_ts_opname(task_data, "")
        self.assertEqual(len(res), 1)

    def test_get_task_based_core_data_1(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(None, None)):
            res = get_task_based_core_data('', DBNameConstant.DB_RUNTIME, {})
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)):
            res = get_task_based_core_data('', DBNameConstant.DB_RUNTIME, {})
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

    def test_get_task_based_core_data_2(self):
        params["data_type"] = StrConstant.AI_CORE_PMU_EVENTS
        create_sql = "CREATE TABLE IF NOT EXISTS " + DBNameConstant.TABLE_AI_CORE_METRIC_SUMMARY + \
                     " (total_time, total_cycles, vec_time, vec_ratio, mac_time, mac_ratio, scalar_time, " \
                     "scalar_ratio, mte1_time, mte1_ratio, mte2_time, mte2_ratio, mte3_time, mte3_ratio, " \
                     "icache_miss_rate, device_id, task_id, stream_id, index_id, model_id)"
        data = ((0.0426514705882353, 58006, 0.019154, 0.449091473295866, 0, 0, 0.000387, 0.00906802744543668, 0, 0,
                 0.010296, 0.241388821846016, 0.019614, 0.459866220735786, 0.0535714285714286, 0, 3, 5, 1, 1),)
        db_manager = DBManager()
        insert_sql = db_manager.insert_sql(DBNameConstant.TABLE_AI_CORE_METRIC_SUMMARY, data)

        test_sql = db_manager.create_table(DBNameConstant.DB_RUNTIME, create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=test_sql), \
                mock.patch(NAMESPACE + '.pre_check_sample', return_value=configs), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch(NAMESPACE + '.add_op_total', return_value=[]), \
                mock.patch(NAMESPACE + '.cal_metrics', return_value=[1, 2]):
            res = get_task_based_core_data('', DBNameConstant.DB_RUNTIME, params)
        test_sql = db_manager.connect_db(DBNameConstant.DB_RUNTIME)
        (test_sql[1]).execute("drop Table {}".format(DBNameConstant.TABLE_AI_CORE_METRIC_SUMMARY))
        db_manager.destroy(test_sql)
        self.assertEqual(res[0], 1)

    def test_get_task_based_core_data_3(self):
        params["data_type"] = StrConstant.AI_VECTOR_CORE_PMU_EVENTS
        create_sql = "CREATE TABLE IF NOT EXISTS " + DBNameConstant.TABLE_AIV_METRIC_SUMMARY + \
                     " ( total_time, total_cycles, vec_time, vec_ratio, mac_time, mac_ratio, scalar_time, " \
                     "scalar_ratio, mte1_time, mte1_ratio, mte2_time, mte2_ratio, mte3_time, mte3_ratio, " \
                     "icache_miss_rate, device_id, task_id, stream_id, index_id, model_id)"
        data = ((0.0426514705882353, 58001, 0.019154, 0.449091473295866, 0, 0, 0.000387, 0.00906802744543668, 0, 0,
                 0.010296, 0.241388821846016, 0.019614, 0.459866220735786, 0.0535714285714286, 0, 3, 5, 1, 1),)
        db_manager = DBManager()
        insert_sql = db_manager.insert_sql(DBNameConstant.TABLE_AIV_METRIC_SUMMARY, data)

        test_sql = db_manager.create_table(DBNameConstant.DB_RUNTIME, create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=test_sql), \
                mock.patch(NAMESPACE + '._get_output_event_counter', return_value=[1, 2]):
            res = get_task_based_core_data('', DBNameConstant.DB_RUNTIME, params)
        test_sql = db_manager.connect_db(DBNameConstant.DB_RUNTIME)
        (test_sql[1]).execute("drop Table {}".format(DBNameConstant.TABLE_AIV_METRIC_SUMMARY))
        db_manager.destroy(test_sql)
        self.assertEqual(res[0], 1)

    def test_get_output_apicall(self):
        db_manager = DBManager()
        test_sql = db_manager.create_table(DBNameConstant.DB_RUNTIME)
        db_manager.destroy(test_sql)
        res = get_output_apicall(None)
        self.assertEqual(res, [])
        test_sql = db_manager.create_table(DBNameConstant.DB_RUNTIME)
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            res = get_output_apicall(test_sql[1])
        self.assertEqual(res, [])

        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=None):
            res = get_output_apicall(test_sql[1])
        self.assertEqual(res, [])
        db_manager.destroy(test_sql)

    def test_get_output_tasktype(self):
        db_manager = DBManager()
        test_sql = db_manager.create_table(DBNameConstant.DB_RUNTIME)

        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=None):
            res = get_output_tasktype(test_sql[1], params)
        self.assertEqual(res, [])
        db_manager.destroy(test_sql)

    def test_get_output_event_counter(self):
        with mock.patch(NAMESPACE + '._get_output_event_counter', return_value=[]):
            res = get_output_tasktype("", params)
        self.assertEqual(res, [])

    def test_get_output_event_counter_1(self):
        with mock.patch(NAMESPACE + '.pre_check_sample', return_value=None):
            res = _get_output_event_counter(None, "", "")
        self.assertEqual(res, [])

        with mock.patch(NAMESPACE + '._get_event_counter_metric_res', side_effect=TypeError):
            res = _get_output_event_counter(None, "", "")
        self.assertEqual(res, [])

        with mock.patch(NAMESPACE + '.pre_check_sample', return_value=configs), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            res = _get_output_event_counter(None, "", "")
        self.assertEqual(res, [])

    def test_get_opname_1(self):
        with mock.patch('os.path.join', side_effect=sqlite3.DatabaseError):
            res = get_opname([], "", "")
        self.assertEqual(res, Constant.NA)

    def test_get_opname_2(self):
        create_ge_sql = "CREATE TABLE IF NOT EXISTS " + DBNameConstant.TABLE_GE_TASK + \
                        " (device_id, model_name, model_id, op_name, stream_id, task_id, block_dim, op_state, " \
                        "task_type, op_type, iter_id, input_count, input_formats, input_data_types, input_shapes, " \
                        "output_count, output_formats, output_data_types, output_shapes)"
        data = ((0, "resnet50", 1, "trans_TransData_0", 5, 3, 1, "static", "AI_CORE", "TransData", 0, 1, "NCHW",
                 "DT_FLOAT16", "1,3,224,224", 1, "NC1HWC0", "DT_FLOAT16", "1,1,224,224,16"),)
        db_manager = DBManager()
        insert_ge_sql = db_manager.insert_sql(DBNameConstant.TABLE_GE_TASK, data)
        res = db_manager.create_table(DBNameConstant.DB_GE_INFO, create_ge_sql, insert_ge_sql, data)

        res_dir = os.path.realpath(os.path.join(db_manager.db_path, ".."))
        res_ge = get_opname([3, 5, -1], res_dir, res[1])
        (res[1]).execute("drop Table {}".format(DBNameConstant.TABLE_GE_TASK))
        db_manager.destroy(res)
        # self.assertEqual(res_ge, 'trans_TransData_0')

    def test_get_opname_3(self):
        with mock.patch('os.path.exists', return_value=False):
            res = get_opname([3, 5, -1], "", "")
        self.assertEqual(res, 'N/A')

    def test_add_op_total_1(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(None, None)):
            res = add_op_total([], "")
        self.assertEqual(res, [])

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)):
            res = add_op_total([], "")
        self.assertEqual(res, [])

    def test_add_op_total_2(self):
        db_manager = DBManager()
        test_sql = db_manager.create_table(DBNameConstant.DB_RTS_TRACK)
        db_manager_ge = DBManager()
        test_ge_sql = db_manager_ge.create_table(DBNameConstant.DB_GE_INFO)
        res_dir = os.path.realpath(os.path.join(db_manager.db_path, ".."))

        with mock.patch(NAMESPACE + ".get_opname", return_value=[1]):
            res = add_op_total([[3, 4, -1]], res_dir)
            res_ge = add_op_total([[3, 4, -1]], res_dir)
        # test_ge_sql = db_manager_ge.connect_db(DB_GE_INFO)
        # test_sql = db_manager.connect_db(DB_RTS_TRACK)
        db_manager_ge.destroy(test_ge_sql)
        db_manager.destroy(test_sql)
        self.assertEqual(len(res), 1)
        self.assertEqual(len(res_ge), 1)

    def test_cal_metrics(self):
        headers = ['Task ID', "Stream ID", "Op Name", "device_id", "x", "mac_ratio", "vec_ratio", "mte2_ratio"]
        result = [[1, 1, "resent", 0, 1, 1, 1, -1, -1, 1, 1]]
        res = cal_metrics(result, [], headers)
        self.assertEqual(len(res), 2)


if __name__ == '__main__':
    unittest.main()

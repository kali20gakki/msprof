import json
import sqlite3
import unittest
from unittest import mock

from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.platform.chip_manager import ChipManager
from constant.constant import ITER_RANGE
from constant.ut_db_name_constant import TABLE_ACL_DATA
from constant.ut_db_name_constant import TABLE_SUMMARY_METRICS
from constant.ut_db_name_constant import TABLE_SUMMARY_TASK_TIME
from profiling_bean.prof_enum.chip_model import ChipModel
from sqlite.db_manager import DBManager
from sqlite.db_manager import DBOpen
from viewer.top_down_report import TopDownData

NAMESPACE = 'viewer.top_down_report'


class TestTopDownData(unittest.TestCase):

    def test_get_acl_data_1(self):
        db = DBManager()
        test_sql = db.create_table('test.db')
        db.destroy(test_sql)
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(None, None)):
            res = TopDownData._get_acl_data('', 0, 0, DBNameConstant.DB_ACL_MODULE)
        self.assertEqual(res, [])

        with mock.patch(NAMESPACE + '.logging.error'), \
             mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=test_sql), \
             mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            TopDownData._get_acl_data('', 0, 0, DBNameConstant.DB_ACL_MODULE)

    def test_get_acl_data_2(self):
        create_sql = "CREATE TABLE IF NOT EXISTS AclData" \
                     " (api_name, api_type, start_time, end_time, process_id, thread_id, device_id)"
        data = (("aclmdlExecute", "model", 7702333934113, 7702338089193, 3251, 3251, 0),)
        db_manager = DBManager()
        insert_sql = db_manager.insert_sql("AclData", data)
        test_sql = db_manager.create_table(DBNameConstant.DB_ACL_MODULE, create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + '.logging.error'), \
             mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=test_sql), \
             mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            res = TopDownData._get_acl_data('', 0, 1, DBNameConstant.DB_ACL_MODULE)
        (test_sql[1]).execute("drop Table AclData")
        db_manager.destroy(test_sql)
        self.assertEqual(len(res), 1)

    def test_get_acl_data_3(self):
        create_sql = "CREATE TABLE IF NOT EXISTS AclData" \
                     " (api_name, api_type, start_time, end_time, process_id, thread_id, device_id)"
        data = (("aclopExecute", "model", 7702333934113, 7702338089193, 3251, 3251, 0),)
        db_manager = DBManager()
        insert_sql = db_manager.insert_sql(DBNameConstant.TABLE_ACL_DATA, data)
        test_sql = db_manager.create_table(DBNameConstant.DB_ACL_MODULE, create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=test_sql), \
             mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            res = TopDownData._get_acl_data('', 0, 1, DBNameConstant.DB_ACL_MODULE)
        (test_sql[1]).execute("drop Table AclData")
        db_manager.destroy(test_sql)
        self.assertEqual(len(res), 1)

    def test_get_ge_data_1(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(None, None)):
            res = TopDownData._get_ge_data('', 0, DBNameConstant.DB_GE_MODEL_INFO)
        self.assertEqual(res, [])
        db = DBManager()
        test_sql = db.create_table('test.db')
        db.destroy(test_sql)
        with mock.patch(NAMESPACE + '.logging.error'), \
             mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=test_sql), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True),\
             mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            res = TopDownData._get_ge_data('', 0, DBNameConstant.DB_GE_MODEL_INFO)
        self.assertEqual(res, [])

        with mock.patch(NAMESPACE + '.logging.error'), \
             mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False), \
             mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            res = TopDownData._get_ge_data('', 0, DBNameConstant.DB_GE_MODEL_INFO)
        self.assertEqual(res, [])

    def test_get_ge_data_2(self):
        create_sql = "CREATE TABLE IF NOT EXISTS " + DBNameConstant.TABLE_GE_MODEL_TIME + \
                     " (modelname, model_id, data_index, req_id, device_id, replayid, input_start, input_end, " \
                     "infer_start, infer_end, output_start, output_end, thread_id)"
        data = (("resnet50", 1, 0, 0, 0, 0, 7702334428334, 7702334470324, 7702334498763, 7702338054533, 7702338070303,
                 7702338071404, 3251),
                ("resnet50", 1, 0, 0, 0, 0, 7702339655870, 7702339688177, 7702339718221, 7702343007338, 7702343022563,
                 7702343023135, 3251))
        db_manager = DBManager()
        insert_sql = db_manager.insert_sql(DBNameConstant.TABLE_GE_MODEL_TIME, data)

        test_sql = db_manager.create_table(DBNameConstant.DB_GE_MODEL_TIME, create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
             mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=test_sql):
            res = TopDownData._get_ge_data('', 0, DBNameConstant.DB_GE_MODEL_TIME)
        test_sql = db_manager.connect_db(DBNameConstant.DB_GE_MODEL_TIME)
        (test_sql[1]).execute("drop Table {}".format(DBNameConstant.TABLE_GE_MODEL_TIME))
        db_manager.destroy(test_sql)
        self.assertEqual(len(res), 3)

    def test_get_runtime_data_1(self):
        db = DBManager()
        test_sql = db.create_table('test.db')
        db.destroy(test_sql)
        with mock.patch(NAMESPACE + '.logging.error'), \
             mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=test_sql), \
             mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            res = TopDownData._get_runtime_data('', 0, DBNameConstant.DB_RUNTIME)
        self.assertEqual(res, [])

        with mock.patch(NAMESPACE + '.logging.error'), \
             mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False), \
             mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            res = TopDownData._get_runtime_data('', 0, DBNameConstant.DB_RUNTIME)
        self.assertEqual(res, [])

    def test_get_runtime_data_2(self):
        create_sql = "CREATE TABLE IF NOT EXISTS ApiCall" \
                     " (replayid, entry_time, exit_time, api, retcode, thread, device_id, " \
                     "stream_id, tasknum, task_id, detail, mode)"
        data = ((0, 117960819118, 117962052296, "KernelLaunch", 0, 2246, 0, 65535, 2, "0,0", None, 0),
                (0, 117962212631, 117962485483, "KernelLaunch", 0, 2246, 0, 4, 1, 0, None, 0))
        db_manager = DBManager()
        insert_sql = db_manager.insert_sql("ApiCall", data)

        test_sql = db_manager.create_table(DBNameConstant.DB_RUNTIME, create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
             mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=test_sql), \
             mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            res = TopDownData._get_runtime_data('', 0, DBNameConstant.DB_RUNTIME)
        (test_sql[1]).execute("drop Table ApiCall")
        db_manager.destroy(test_sql)
        self.assertEqual(len(res), 1)

    def test_get_task_scheduler_1(self):
        db = DBManager()
        test_sql = db.create_table('test.db')
        db.destroy(test_sql)
        with mock.patch(NAMESPACE + '.logging.error'), \
             mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=test_sql), \
             mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            res = TopDownData._get_task_scheduler('', 0)
        self.assertEqual(res, [])

        with mock.patch(NAMESPACE + '.logging.error'), \
             mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(None, None)), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False), \
             mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            res = TopDownData._get_task_scheduler('', 0)
        self.assertEqual(res, [])

        with mock.patch(NAMESPACE + '.logging.error'), \
             mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False), \
             mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            res = TopDownData._get_task_scheduler('', 0)
        self.assertEqual(res, [])

    def test_get_task_scheduler_2(self):
        create_sql = "CREATE TABLE IF NOT EXISTS " + TopDownData.OP_SUMMARY_TASK_TIME_TABLE + \
                     " (task_id, stream_id, start_time, duration_time, wait_time, device_id, index_id, model_id)"
        data = ((2, 3, 7702334449989, 3430937, 0, 0, 1, 1),)
        db_manager = DBManager()
        insert_sql = db_manager.insert_sql(TopDownData.OP_SUMMARY_TASK_TIME_TABLE, data)

        test_sql = db_manager.create_table(DBNameConstant.DB_AICORE_OP_SUMMARY, create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
             mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=test_sql), \
             mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            res = TopDownData._get_task_scheduler('', 1)
        (test_sql[1]).execute("drop Table {}".format(TopDownData.OP_SUMMARY_TASK_TIME_TABLE))
        db_manager.destroy(test_sql)
        self.assertEqual(len(res), 1)

    def test_get_top_down_data_one_iter(self):
        with mock.patch(NAMESPACE + '.TopDownData._get_acl_data', return_value=[]), \
             mock.patch(NAMESPACE + '.TopDownData._get_ge_data', return_value=[]), \
             mock.patch(NAMESPACE + '.TopDownData._get_runtime_data', return_value=[]), \
             mock.patch(NAMESPACE + '.TopDownData._get_task_scheduler', return_value=[]):
            res = TopDownData._get_top_down_data_one_iter("", 0, 1)
        self.assertEqual(res, [])

    def test_fill_top_down_trace_data(self):
        with mock.patch(NAMESPACE + '.TopDownData._dispatch_top_down_datas'), \
             mock.patch(NAMESPACE + '.TopDownData._fill_acl_trace_data'), \
             mock.patch(NAMESPACE + '.TopDownData._fill_ge_trace_data'), \
             mock.patch(NAMESPACE + '.TopDownData._fill_runtime_trace_data'), \
             mock.patch(NAMESPACE + '.TopDownData._fill_ts_trace_data'):
            TopDownData._fill_top_down_trace_data("", 0, [], [])

    def test_dispatch_top_down_datas(self):
        data = [(1, 'ACL', 'aclmdlExecute', 7702333934113, 4155080), (1, 'GE', 'Input', 7702334428334, 41990),
                (1, 'GE', 'Infer', 7702334498763, 3555770), (1, 'GE', 'Output', 7702338070303, 1101),
                (1, 'Runtime', 'N/A', 7702311524052, 13106532)]
        res = TopDownData._dispatch_top_down_datas(data)
        self.assertEqual(len(res), 4)

    def test_fill_acl_trace_data_1(self):
        top_down_datas = {"ACL": [(1, 'ACL', 'aclmdlExecute', 7702333934113, 4155080)],
                          "GE": [(1, 'GE', 'Input', 7701334428334, 41990),
                                 (1, 'GE', 'Infer', 7702334498763, 3555770), (1, 'GE', 'Output', 7702338070303, 1101),
                                 (1, 'GE', 'Output', 7702338070303, 7702338070303)],
                          "Task Scheduler": [(1, 'Task Scheduler', 'Infer', 7702334498763, 3555770)],
                          "Runtime": [(1, 'Runtime', 'N/A', 7702311524052, 13106532)]}

        create_sql = "CREATE TABLE IF NOT EXISTS AclData" \
                     " (api_name, api_type, start_time, end_time, process_id, thread_id, device_id)"
        data = (("aclmdlQuerySize", "model", 7702334428334, 7703334428334, 3251, 3251, 0),)
        db_manager = DBManager()
        insert_sql = db_manager.insert_sql("AclData", data)
        test_sql = db_manager.create_table(DBNameConstant.DB_ACL_MODULE, create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=test_sql), \
             mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            InfoConfReader()._info_json = {"pid": 1}
            TopDownData._fill_acl_trace_data('', 0, [], top_down_datas)

        (test_sql[1]).execute("DELETE from AclData")
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=test_sql), \
             mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            InfoConfReader()._info_json = {"pid": 1}
            TopDownData._fill_acl_trace_data('', 0, [], top_down_datas)
        (test_sql[1]).execute("drop Table AclData")
        db_manager.destroy(test_sql)

    def test_fill_acl_trace_data_2(self):
        top_down_datas = {"ACL": [(1, 'ACL', 'aclmdlExecute', 7702333934113, 4155080)]}
        TopDownData._fill_acl_trace_data('', 0, [], top_down_datas)

    def test_fill_ge_trace_data(self):
        top_down_datas = [(1, 'GE', 'Input', 7702334428334, 41990),
                          (1, 'GE', 'Infer', 7702334498763, 3555770),
                          (1, 'GE', 'Output', 7702338070303, 1101)]
        InfoConfReader()._info_json = {"pid": 1}
        TopDownData._fill_ge_trace_data([], top_down_datas)

    def test_fill_runtime_trace_data_1(self):
        with mock.patch(NAMESPACE + '.logging.error'), \
             mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
             mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            TopDownData._fill_runtime_trace_data('', [], [])

    def test_fill_runtime_trace_data_2(self):
        top_down_datas = [(1, 'Runtime', 'N/A', 7702311524052, 13106532)]
        create_sql = "CREATE TABLE IF NOT EXISTS ApiCall" \
                     " (replayid, entry_time, exit_time, api, retcode, thread, device_id, stream_id, " \
                     "tasknum, task_id, detail, mode)"
        data = ((0, 7702311524053, 7702311524054, 6, 0, 3251, 0, 2, 1, 0, None, 0),)
        db_manager = DBManager()
        insert_sql = db_manager.insert_sql("ApiCall", data)

        test_sql = db_manager.create_table(DBNameConstant.DB_RUNTIME, create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=test_sql), \
             mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            InfoConfReader()._info_json = {"pid": 1}
            TopDownData._fill_runtime_trace_data('', [], top_down_datas)
        (test_sql[1]).execute("drop Table ApiCall")
        db_manager.destroy(test_sql)

    def test_fill_ts_trace_data_1(self):
        with mock.patch(NAMESPACE + '.logging.error'), \
             mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
             mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            TopDownData._fill_ts_trace_data('', [], [])

        with mock.patch(NAMESPACE + '.logging.error'), \
             mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False), \
             mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            TopDownData._fill_ts_trace_data('', [], [1])

    # def test_fill_ts_trace_data_2(self): XXX
    #     top_down_datas = [(1, 'Runtime', 'N/A', 7702311524052, 13106532)]
    #     create_op_sql = "CREATE TABLE IF NOT EXISTS " + TopDownData.OP_SUMMARY_METRICS + \
    #                     " (total_time, total_cycles, vec_time, vec_ratio, mac_time, mac_ratio, scalar_time, " \
    #                     "scalar_ratio, mte1_time, mte1_ratio, mte2_time, mte2_ratio, mte3_time, mte3_ratio, " \
    #                     "icache_miss_rate, device_id, task_id, stream_id, index_id)"
    #     create_acl_sql = "CREATE TABLE IF NOT EXISTS acl_data (name, device_id)"
    #     create_ts_sql = "CREATE TABLE IF NOT EXISTS " + TopDownData.OP_SUMMARY_TASK_TIME_TABLE + \
    #                     " (task_id, stream_id, start_time, duration_time, wait_time, device_id, index_id)"

    #     op_data = ((0.017485, 559511, 0.000157, 0.00897926939774196, 0, 0, 9.2E-5, 0.00527424840619755, 0, 0, 0.011119,
    #                 0.635901707026314, 0.002002, 0.114508919395687, 0.111111111111111, 0, 1, 1, 1),)
    #     acl_data = (("aclpExecute", 0),)
    #     ts_data = ((1, 1, 18911666480340, 10, 41060851790, 0, 1),)

    #     with DBOpen(DBNameConstant.DB_AICORE_OP_SUMMARY) as db_open:
    #         db_open.create_table(create_acl_sql)
    #         db_open.insert_data(TABLE_ACL_DATA, acl_data)
    #         db_open.create_table(create_ts_sql)
    #         db_open.insert_data(TABLE_SUMMARY_TASK_TIME, ts_data)
    #         db_open.create_table(create_op_sql)
    #         db_open.insert_data(TABLE_SUMMARY_METRICS, op_data)
    #         with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(db_open.db_conn, db_open.db_curs)), \
    #              mock.patch(NAMESPACE + '.TopDownData._get_op_name', return_value={}), \
    #              mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
    #             InfoConfReader()._info_json = {"pid": 1}
    #             TopDownData._fill_ts_trace_data('', [], top_down_datas)

    # def test_get_op_name(self): XXX
    #     create_sql = "CREATE TABLE IF NOT EXISTS " + DBNameConstant.TABLE_GE_TASK + \
    #                  " (device_id, model_name, model_id, op_name, stream_id, task_id, block_dim, op_state, " \
    #                  "task_type, op_type, iter_id, input_count, input_formats, input_data_types, input_shapes," \
    #                  " output_count, output_formats, output_data_types, output_shapes, timestamp)"
    #     data = ((0, "ge_default_20210326080919_1", 1, "global_step/Assign", 1, 1, 1, "static", "AI_CORE", "Assign",
    #              0, 2, "ND;ND", "DT_INT64;DT_INT64", ";", 1, "ND", "DT_INT64", "", 123456789),)

    #     with DBOpen(DBNameConstant.DB_GE_INFO) as db_open:
    #         db_open.create_table(create_sql)
    #         db_open.insert_data(DBNameConstant.TABLE_GE_TASK, data)
    #         with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(db_open.db_conn, db_open.db_curs)), \
    #              mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
    #             InfoConfReader()._info_json = {"pid": 1}
    #             res = TopDownData._get_op_name("")
    #         self.assertEqual(res, {'1_1_0': 'global_step/Assign'})

    # def test_get_total_time(self): XXX
    #     create_sql = "CREATE TABLE IF NOT EXISTS " + DBNameConstant.TABLE_METRICS_SUMMARY \
    #                  + "(stream_id, task_id, total_time)"
    #     data = ((1, 1, 12.23), (1, 2, 12.35))
    #     with DBOpen(DBNameConstant.DB_RUNTIME) as db_open:
    #         db_open.create_table(create_sql)
    #         db_open.insert_data(DBNameConstant.TABLE_METRICS_SUMMARY, data)
    #         with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(db_open.db_conn, db_open.db_curs)), \
    #              mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'),\
    #              mock.patch('os.path.exists', return_value=True):
    #             res = TopDownData._get_total_time("")
    #         self.assertEqual(res, {'1_1_0': 12.23, '1_2_0': 12.35})

    def test_get_top_down_data_1(self):
        with mock.patch(NAMESPACE + '.TopDownData._get_top_down_data_one_iter', return_value=[]):
            res = TopDownData.get_top_down_data("", '0', ITER_RANGE)
        self.assertEqual(res[2], 0)

    def test_get_top_down_timeline_data_1(self):
        with mock.patch(NAMESPACE + '.TopDownData._get_top_down_data_one_iter', return_value=[1]), \
             mock.patch(NAMESPACE + '.TopDownData._fill_top_down_trace_data', return_value=[1]):
            InfoConfReader()._info_json = {"pid": 1}
            res = TopDownData.get_top_down_timeline_data("", '0', ITER_RANGE)
        self.assertEqual(res, "")

        with mock.patch(NAMESPACE + '.TopDownData._get_top_down_data_one_iter', return_value=ValueError):
            res = TopDownData.get_top_down_timeline_data("", '0', ITER_RANGE)
        self.assertEqual(res, "")


if __name__ == '__main__':
    unittest.main()

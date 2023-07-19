import unittest
from collections import deque
from unittest import mock

from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.constant import Constant
from common_func.data_manager import DataManager
from common_func.db_name_constant import DBNameConstant
from common_func.msvp_constant import MsvpConstant
from constant.ut_db_name_constant import DB_AICORE_OP_SUMMARY
from constant.ut_db_name_constant import DB_OP_COUNTER
from constant.ut_db_name_constant import TABLE_AI_CPU
from constant.ut_db_name_constant import TABLE_OP_COUNTER_OP_REPORT
from sqlite.db_manager import DBManager
from sqlite.db_manager import DBOpen
from viewer.ai_core_op_report import AiCoreOpReport
from viewer.ai_core_op_report import ReportOPCounter

NAMESPACE = 'viewer.ai_core_op_report'
config = {'handler': '_get_op_summary_data',
          'headers': ['Model Name', 'Model ID', 'Task ID', 'Stream ID', 'Infer ID', 'Op Name', 'OP Type',
                      'Task Type', 'Task Start Time', 'Task Duration(us)', 'Task Wait Time(us)', 'Block Dim'],
          'db': 'ai_core_op_summary.db', 'unused_cols': []}


class TestAiCoreOpReport(unittest.TestCase):
    def test_get_op_summary_data1(self):
        with mock.patch(NAMESPACE + '.AiCoreOpReport.get_ai_core_op_summary_data', return_value=[]), \
                mock.patch(NAMESPACE + '.AiCoreOpReport.get_hardware_op_summary_data', return_value=[]):
            check = AiCoreOpReport()
            res = check.get_op_summary_data("0", "", config)
        expect_res = [
            'Model Name', 'Model ID', 'Task ID', 'Stream ID', 'Infer ID', 'Op Name', 'OP Type',
            'Task Type', 'Task Start Time', 'Task Duration(us)', 'Task Wait Time(us)', 'Block Dim'
        ]
        self.assertEqual(res[0], expect_res)
        self.assertEqual(res[1], [])
        self.assertEqual(res[2], 0)

    def test_get_hardware_op_summary_data_error(self):
        res = AiCoreOpReport.get_hardware_op_summary_data('', '', [], {})
        self.assertEqual(res, [])

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(1, 1)), \
                mock.patch(NAMESPACE + '.AiCoreOpReport.get_hardware_op_data', side_effect=TypeError):
            res = AiCoreOpReport.get_hardware_op_summary_data('', '', [], {})
        self.assertEqual(res, [])
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(1, 1)), \
                mock.patch(NAMESPACE + '.AiCoreOpReport.get_hardware_op_data', return_value=[]):
            res = AiCoreOpReport.get_hardware_op_summary_data('', '', [], {})
        self.assertEqual(res, [])

    def test_get_hardware_op_summary_data_normal(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(1, 1)), \
                mock.patch(NAMESPACE + '.AiCoreOpReport.get_hardware_op_data', return_value=[]):
            res = AiCoreOpReport.get_hardware_op_summary_data('', '', [], {})
        self.assertEqual(res, [])

    def test_get_ai_core_op_summary_data(self):
        with mock.patch(NAMESPACE + '.AiCoreOpReport._get_op_summary_data', return_value=[]):
            check = AiCoreOpReport()
            ProfilingScene()._scene = Constant.SINGLE_OP
            res = check.get_ai_core_op_summary_data('', '', {})
        self.assertEqual(res, [])

        with mock.patch(NAMESPACE + '.AiCoreOpReport._get_op_summary_data', return_value=[]):
            check = AiCoreOpReport()
            ProfilingScene().init('')
            ProfilingScene()._scene = Constant.STEP_INFO
            res = check.get_ai_core_op_summary_data('', '', {})
        self.assertEqual(res, [])

    def test__get_hardware_op_datas_will_return_merged_data_when_dvpp_and_dsa_are_not_empty(self):
        with mock.patch(NAMESPACE + '.AiCoreOpReport._get_hardware_op_sql_data', return_value=[[1, 1, 1]]):
            check = AiCoreOpReport()
            self.assertEqual(len(check._get_hardware_op_datas(None)), 2)

    def test_get_ai_core_op_summary_data_1(self):
        with mock.patch(NAMESPACE + '.AiCoreOpReport._get_op_summary_data', side_effect=TypeError):
            res = AiCoreOpReport.get_ai_core_op_summary_data('', '', {})
        self.assertEqual(res, [])

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(None, None)), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            res = AiCoreOpReport.get_ai_core_op_summary_data('', '', {})
        self.assertEqual(res, [])

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(1, 2)), \
                mock.patch(NAMESPACE + '.AiCoreOpReport._get_op_summary_data', return_value=[]), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            ProfilingScene().init('')
            ProfilingScene()._scene = Constant.STEP_INFO
            res = AiCoreOpReport.get_ai_core_op_summary_data('', '', {})
        self.assertEqual(res, [])

    def test_update_model_name_and_infer_id(self):
        data = [[1, 2, 9, 'global_step/Assign', 'Assign', 'AI_CORE',
                 99655949576850, 3.4, 0.0, 1, '', 'INT64;INT64', 'FORMAT_ND;FORMAT_ND', '',
                 'INT64', 'FORMAT_ND', 0.002988, 2988, 0, 0, 0, 0, 0, 0, 0, 0]]
        with mock.patch(NAMESPACE + '.get_ge_model_name_dict', return_value={}):
            res = AiCoreOpReport._update_model_name_and_infer_id('', data)
        self.assertEqual(len(res), 1)

    def test_update_op_name_from_hash(self):
        data = [[1, 2, 9, 'global_step/Assign', 'Assign', 'AI_CORE',
                 99655949576850, 3.4, 0.0, 1, '', 'INT64;INT64', 'FORMAT_ND;FORMAT_ND', '',
                 'INT64', 'FORMAT_ND', 0.002988, 2988, 0, 0, 0, 0, 0, 0, 0, 0]]
        with mock.patch(NAMESPACE + '.get_ge_hash_dict', return_value={}):
            res = AiCoreOpReport._update_op_name_from_hash('', data)
        self.assertEqual(len(res), 1)

    def test_get_aicore_data(self):
        with mock.patch(NAMESPACE + '.AiCoreOpReport._count_num', return_value=2), \
                mock.patch(NAMESPACE + '.AiCoreOpReport._get_used_headers', return_value=[]), \
                mock.patch(NAMESPACE + '.AiCoreOpReport._get_ai_core_float_cols', return_value=[]), \
                mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            headers = config.get("headers")
            res = AiCoreOpReport._get_aicore_data('', headers)
        self.assertEqual(res[0], {})

        with mock.patch(NAMESPACE + '.AiCoreOpReport._count_num', return_value=2), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            headers = config.get("headers")
            res = AiCoreOpReport._get_aicore_data('', headers)
        self.assertEqual(res[0], {})

    def test_get_two_table_union_sql(self):
        res_sql_1 = "select ge_summary.model_id, task_time.task_id, task_time.stream_id,  op_name, " \
                    "ge_summary.op_type, ge_summary.task_type, start_time/1000.0, duration_time/1000.0, " \
                    "wait_time/1000.0, block_dim, mix_block_dim, " \
                    "(case when context_id=4294967295 then 'N/A' else context_id end) " \
                    "from task_time inner join ge_summary on task_time.task_id=ge_summary.task_id " \
                    "and task_time.stream_id = ge_summary.stream_id " \
                    "and ge_summary.task_type!=? " \
                    "and ge_summary.task_type!=? and ge_summary.task_type!=? " \
                    "and task_time.batch_id=ge_summary.batch_id and ge_summary.context_id=task_time.subtask_id " \
                    "and task_time.start_time != -1 " \
                    "order by start_time"
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            ProfilingScene().init('')
            ProfilingScene()._scene = Constant.SINGLE_OP
            res = AiCoreOpReport._get_two_table_union_sql()
            self.assertEqual(res, res_sql_1)

    def test_get_sql_and_headers(self):
        judge_result = (False, False)
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch(NAMESPACE + '.AiCoreOpReport._get_tensor_table_sql_and_headers', return_value=(1, 2)):
            headers = config.get("headers")
            res = AiCoreOpReport._get_sql_and_headers('', headers)
        self.assertEqual(res[0], 1)

        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', side_effect=judge_result), \
                mock.patch(NAMESPACE + '.AiCoreOpReport._get_table_sql_and_headers_without_ge', return_value=1):
            headers = config.get("headers")
            res = AiCoreOpReport._get_sql_and_headers('', headers)
        self.assertEqual(res, 1)

        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', side_effect=(False, True)), \
                mock.patch(NAMESPACE + '.AiCoreOpReport._get_two_table_union_sql', return_value=1):
            headers = config.get("headers")
            res = AiCoreOpReport._get_sql_and_headers('', headers)
        self.assertEqual(res[0], 1)

    def test_get_op_summary_data(self):
        create_ge_sql = "CREATE TABLE IF NOT EXISTS ge_summary(model_name text,model_id INTEGER,task_id INTEGER," \
                        "stream_id INTEGER, op_name text,op_type text,block_dim INTEGER,input_shapes text," \
                        "input_data_types text,input_formats text,output_shapes text,output_data_types text," \
                        "output_formats text,device_id INTEGER,task_type text,index_id INTEGER)"
        union_sql = "select * from ge_summary where ge_summary.index_id=? and ge_summary.task_type!=?"
        ge_data = ((0, 4294967295, 3, 6, "TransData", "TransData", 32, "1,512,1,1", "DT_FLOAT", "NCHW", "1,32,1,1,16",
                    "DT_FLOAT", "NC1HWC0", 0, "AI_CORE", 1),)
        with DBOpen(DB_AICORE_OP_SUMMARY) as db_open:
            db_open.create_table(create_ge_sql)
            db_open.insert_data(DBNameConstant.TABLE_SUMMARY_GE, ge_data)
            with mock.patch(NAMESPACE + '.AiCoreOpReport._get_sql_and_headers', return_value=(union_sql, [])), \
                    mock.patch(NAMESPACE + '.AiCoreOpReport._get_aicore_data', return_value=([], [])), \
                    mock.patch(NAMESPACE + '.AiCoreOpReport._union_task_ge_ai_core_data', return_value=[]), \
                    mock.patch(NAMESPACE + '.AiCoreOpReport._update_model_name_and_infer_id', return_value=[]), \
                    mock.patch(NAMESPACE + '.AiCoreOpReport._update_op_name_from_hash', return_value=[]), \
                    mock.patch(NAMESPACE + '.add_aicore_units', return_value=[]):
                headers = config.get("headers")
                ProfilingScene().init('')
                ProfilingScene()._scene = Constant.SINGLE_OP
                res = AiCoreOpReport._get_op_summary_data('', db_open.db_curs, headers)
            self.assertEqual(res, [])

    def test_union_task_ge_ai_core_data(self):
        expect_res = [(1, 2, 3, 4, 10), (4, 5, 6, 7, 40), (7, 8, 9, 10, 'N/A')]
        data = [(1, 2, 3, 4), (4, 5, 6, 7), (7, 8, 9, 10)]
        ai_core_group_dict = {(2, 3, 4): deque([(10,)]), (5, 6, 7): deque([(40,)]), (10, 11, 12): deque([(20,)])}
        res = AiCoreOpReport._union_task_ge_ai_core_data(data, ai_core_group_dict)
        self.assertEqual(res, expect_res)

    def test_count_num(self):
        curs = mock.Mock()
        curs.execute.return_value = mock.Mock()
        curs.execute.return_value.fetchone.return_value = [0]
        res = AiCoreOpReport._count_num("test", curs)
        self.assertEqual(res, 0)

    def test_get_used_headers(self):
        with mock.patch(NAMESPACE + ".DBManager.fetch_all_data",
                        return_value=[[None, "r1"], [None, "r2"], [None, "1"], [None, "2"]]):
            res = AiCoreOpReport._get_used_headers(mock.Mock(), "test", ["1", "2"])
        self.assertEqual(res, ["r1", "r2"])

    def test_get_ai_core_float_cols(self):
        with mock.patch(NAMESPACE + ".read_cpu_cfg", return_value={1: "r1", 2: "r2_ratio"}):
            res = AiCoreOpReport._get_ai_core_float_cols(["r1", "r2_time"])
        self.assertEqual(res, ['round(r1, 6)', 'round(r2_time, 6)'])

    def test_add_memory_bound(self):
        headers = ["mac_ratio", "vec_ratio", "mte2_ratio"]
        data = [[1, 2, 3], [5, 6, 2]]
        DataManager.add_memory_bound(headers, data)
        headers = ["mac_exe_ratio", "vec_exe_ratio", "mte2_exe_ratio"]
        data = [[1, 2, 3], [5, 6, 2]]
        DataManager.add_memory_bound(headers, data)

    def test_get_table_sql_and_headers_without_ge(self):
        res_data = (
            "select -1,  task_id, stream_id,  'N/A', 'N/A', task_type, start_time/1000.0, duration_time/1000.0, "
            "wait_time/1000.0 ,'N/A' "
            "from task_time where task_type!=? and task_type!=? and task_type!=? order by start_time",
            ['Op Name', 'stream_id']
        )
        ProfilingScene().init('')
        ProfilingScene()._scene = Constant.SINGLE_OP
        res = AiCoreOpReport._get_table_sql_and_headers_without_ge(["Op Name", "stream_id", "Block Dim"])
        self.assertEqual(res, res_data)


class TestReportOPCounter(unittest.TestCase):
    def test_check_param(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            res = ReportOPCounter.check_param('', '')
        self.assertEqual(res, False)

    def test_report_op_1(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(None, None)):
            res = ReportOPCounter.report_op('', [])
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

    def test_report_op_2(self):
        create_sql = "CREATE TABLE IF NOT EXISTS " + TABLE_OP_COUNTER_OP_REPORT + \
                     "(model_name, op_type, core_type, occurrences, total_time, " \
                     "min, avg, max, ratio, device_id)"
        data = ((1, 4, 1695387952406, 1695385794913, 59, 4, 1695386978300, 59, 103, 0),)

        with DBOpen(DB_OP_COUNTER) as db_open:
            db_open.create_table(create_sql)
            db_open.insert_data(TABLE_OP_COUNTER_OP_REPORT, data)
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path',
                            return_value=(db_open.db_conn, db_open.db_curs)), \
                    mock.patch(NAMESPACE + '.isinstance', return_value=False):
                ProfilingScene()._scene = Constant.STEP_INFO
                res = ReportOPCounter.report_op('', [])
            self.assertEqual(res[2], 1)

        with DBOpen(DB_OP_COUNTER) as db_open:
            db_open.create_table(create_sql)
            db_open.insert_data(TABLE_OP_COUNTER_OP_REPORT, data)
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path',
                            return_value=(db_open.db_conn, db_open.db_curs)), \
                    mock.patch(NAMESPACE + '.isinstance', return_value=False):
                check = ReportOPCounter()
                ProfilingScene().init('')
                ProfilingScene()._scene = Constant.SINGLE_OP
                res = check.report_op('', config.get("headers"))
            self.assertEqual(res[2], 1)


if __name__ == '__main__':
    unittest.main()

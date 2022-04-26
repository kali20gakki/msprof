import sqlite3
import unittest
from collections import OrderedDict

from unittest import mock
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.constant import Constant
from sqlite.db_manager import DBManager
from viewer.training.core_cpu_reduce_viewer import CoreCpuReduceViewer
from analyzer.scene_base.profiling_scene import ProfilingScene

NAMESPACE = 'viewer.training.core_cpu_reduce_viewer'


class TestCoreCpuReduceViewer(unittest.TestCase):
    def setUp(self):
        _db_manager = DBManager()
        _db_manager.destroy(_db_manager.create_sql(DBNameConstant.DB_TRACE))

    def tearDown(self):
        _db_manager = DBManager()
        _db_manager.destroy(_db_manager.create_sql(DBNameConstant.DB_TRACE))

    def test_get_task_scheduler_data(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(True, True)), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            res = CoreCpuReduceViewer._get_task_scheduler_data("1")
        self.assertEqual(res, [])

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(True, True)), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
             mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[[1]]), \
             mock.patch(NAMESPACE + '.TraceViewManager.time_graph_trace', return_value=[[1]]), \
             mock.patch(NAMESPACE + '.CoreCpuReduceViewer._format_task_trace_data',
                        return_value=[[1]]), \
             mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            res = CoreCpuReduceViewer._get_task_scheduler_data("1")
        self.assertEqual(len(res), 3)

    def test_format_task_trace_data(self):
        sql_datas = [(3, 2, 173584344597.04166, 24557.29168701172, "AI_CORE", 1, 1)]
        opnames = {'3_2_AI_CORE_1': '1'}
        task_types = {'3_2_AI_CORE_1': 'AI_CORE'}
        total_time = {'3_2_AI_CORE_1': 0.02}
        total_cycle = {'3_2_AI_CORE_1': "N/A"}
        result = [['1', 0, 3, 173584344.59704167, 24.557,
                   OrderedDict([('Task Type', 'AI_CORE'), ('Stream Id', 3), ('Task Id', 2),
                                ('Aicore Time(ms)', 0.02), ('Total Cycle', 'N/A')])]]
        with mock.patch(NAMESPACE + '.CoreCpuReduceViewer._get_op_names_and_task_type',
                        return_value=(opnames, task_types)), \
             mock.patch(NAMESPACE + '.CoreCpuReduceViewer._get_total_cycle', return_value=(total_cycle, total_time)):
            res = CoreCpuReduceViewer._format_task_trace_data(sql_datas, DBManager().db_path)
        self.assertEqual(res, result)

    def test_get_ai_cpu_data(self):
        ai_cpu_data = [(1686766372751, 'resnet_model',
                        2.309, 0.006, 2.36362, 0.131, 3.418)]

        with mock.patch(NAMESPACE + '.ParseAiCpuData.analysis_aicpu', return_value=(None, None)):
            res = CoreCpuReduceViewer._get_ai_cpu_data(0, 1)
        self.assertEqual(res, [])

        with mock.patch(NAMESPACE + '.ParseAiCpuData.analysis_aicpu', return_value=(None, ai_cpu_data)):
            info = InfoConfReader()
            info._info_json = {'DeviceInfo': [{'hwts_frequency': 100}]}
            res = CoreCpuReduceViewer._get_ai_cpu_data(0, 1)
        self.assertEqual(len(res), 2)

    def test_get_all_reduce_data_1(self):

        with mock.patch(NAMESPACE + '.PathManager.get_sql_dir', return_value=DBManager().db_path), \
             mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(None, None)), \
             mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            res = CoreCpuReduceViewer._get_all_reduce_data(0, 1, 1, result_dir='')
        self.assertEqual(res, [])

    def test_get_all_reduce_data_2(self):
        training_trace_data = ((1, 1695384646479, 1695386378943, 1695387952406, 3305927, 1732464, 1573463, 0),)

        create_sql = "CREATE TABLE IF NOT EXISTS " + DBNameConstant.TABLE_TRAINING_TRACE + \
                     "(iteration_id, FP_start, BP_end, iteration_end, iteration_time, " \
                     "fp_bp_time, grad_refresh_bound, data_aug_bound)"
        insert_sql = "insert into {} values (?,?,?,?,?,?,?,?)".format(DBNameConstant.TABLE_TRAINING_TRACE)
        db_manager = DBManager()
        db_manager.create_table(DBNameConstant.DB_TRACE, create_sql, insert_sql, training_trace_data)

        result = [OrderedDict([('name', 'process_name'), ('pid', 2), ('tid', 0),
                               ('args', OrderedDict([('name', 'All Reduce')])), ('ph', 'M')]),
                  OrderedDict([('name', 'AR 0'), ('pid', 2), ('tid', 0), ('ts', 16953857949.130001),
                               ('dur', 11833.869999999999),
                               ('args', OrderedDict([('Reduce Duration(us)', 11833.869999999999)])), ('ph', 'X')])]

        create_sql = "CREATE TABLE IF NOT EXISTS " + DBNameConstant.TABLE_ALL_REDUCE + \
                     "(host_id int, device_id int, iteration_end int, start int, start_stream, start_task, " \
                     "end, end_stream, end_task)"
        insert_sql = "insert into {} values (?,?,?,?,?,?,?,?,?)".format(DBNameConstant.TABLE_ALL_REDUCE)
        data = ((1, 4, 1695387952406, 1695385794913, 59, 4, 1695386978300, 59, 103),)
        db_manager = DBManager()
        test_sql = db_manager.create_table(DBNameConstant.DB_TRACE, create_sql, insert_sql, data)

        with mock.patch(NAMESPACE + '.PathManager.get_sql_dir', return_value=db_manager.db_path), \
             mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=test_sql), \
             mock.patch(NAMESPACE + '.CoreCpuReduceViewer._get_trace_one_device', return_value=training_trace_data), \
             mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            info = InfoConfReader()
            info._info_json = {'DeviceInfo': [{'hwts_frequency': 100}]}
            res = CoreCpuReduceViewer._get_all_reduce_data(4, 1, 1, result_dir='')
        (test_sql[1]).execute("drop Table {}".format(DBNameConstant.TABLE_ALL_REDUCE))
        db_manager.destroy(test_sql)
        self.assertEqual(res, result)

    def test_format_all_reduce_data(self):
        with mock.patch(NAMESPACE + '.CoreCpuReduceViewer._get_trace_one_device', return_value=[]):
            InfoConfReader()._info_json = {'DeviceInfo': [{'hwts_frequency': 100}]}
            res = CoreCpuReduceViewer._format_all_reduce_data('', 0, 1, 1)
        self.assertEqual(res, [])

    def test_get_trace_one_device(self):
        result = [(2, 4, 1695386978300, 59, 103, 1, 1, 1)]
        create_sql = "CREATE TABLE IF NOT EXISTS " + DBNameConstant.TABLE_TRAINING_TRACE + \
                     "(host_id int, device_id int, iteration_id int, job_stream, job_task, " \
                     " FP_start, BP_end, iteration_end, iteration_time, " \
                     "fp_bp_time, grad_refresh_bound, data_aug_bound, model_id)"
        data = ((1, 4, 2, 1695385794913, 59, 4, 1695386978300, 59, 103, 1, 1, 1, 1),)
        insert_sql = "insert into {0} values ({1})".format(DBNameConstant.TABLE_TRAINING_TRACE,
                                                           "?," * (len(data[0]) - 1) + "?")
        db_manager = DBManager()
        test_sql = db_manager.create_table(DBNameConstant.DB_TRACE, create_sql, insert_sql, data)

        ProfilingScene().init("")
        ProfilingScene()._scene = Constant.STEP_INFO
        res = CoreCpuReduceViewer._get_trace_one_device(test_sql[1], 4, 2, 1)
        self.assertEqual(res, result)

        ProfilingScene().init("")
        ProfilingScene()._scene = Constant.TRAIN
        res = CoreCpuReduceViewer._get_trace_one_device(test_sql[1], 4, 2, 1)
        self.assertEqual(res, result)

        (test_sql[1]).execute("drop Table {}".format(DBNameConstant.TABLE_TRAINING_TRACE))
        db_manager.destroy(test_sql)

    def test_get_op_names_and_task_type_1(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(True, True)), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            res = CoreCpuReduceViewer._get_op_names_and_task_type('')
        self.assertEqual(res[0], {})

    def test_get_op_names_and_task_type_2(self):
        ge_data = [("a", 3, 2, "AI_CORE", 1)]
        result = {"3_2_AI_CORE_1": "a"}
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(True, True)), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
             mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=ge_data), \
             mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            res = CoreCpuReduceViewer._get_op_names_and_task_type('')
        self.assertEqual(result, res[0])

    def test_get_total_cycle_1(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(True, True)), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            res = CoreCpuReduceViewer._get_total_cycle('')
        self.assertEqual(res[0], dict())

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(True, True)), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            res = CoreCpuReduceViewer._get_total_cycle('')
        self.assertEqual(res[0], dict())

    def test_get_total_cycle_2(self):
        result = {'12_2_AI_CORE_0': 3450}
        create_sql = "CREATE TABLE IF NOT EXISTS " + DBNameConstant.TABLE_AI_CORE_METRIC_SUMMARY + \
                     "(total_cycles, stream_id, task_id, index_id, total_time)"
        insert_sql = "insert into {} values (?,?,?,?,?)".format(DBNameConstant.TABLE_AI_CORE_METRIC_SUMMARY)
        data = ((3450, 12, 2, 1, 0.00345),)
        db_manager = DBManager()
        test_sql = db_manager.create_table(DBNameConstant.DB_RUNTIME, create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=test_sql), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
             mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            res = CoreCpuReduceViewer._get_total_cycle("")
        self.assertEqual(res[0], result)

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=test_sql), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
             mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            (test_sql[1]).execute("delete from MetricSummary")
            res = CoreCpuReduceViewer._get_total_cycle("")
        (test_sql[1]).execute("drop Table MetricSummary")
        test_sql[0].commit()
        db_manager.destroy(test_sql)
        self.assertEqual(res[0], dict())

    def test_get_task_time_data(self):
        params = {"device_id": 0,
                  "iter_id": 1,
                  "result_dir": "result_dir"}
        with mock.patch(NAMESPACE + '.CoreCpuReduceViewer._get_task_scheduler_data', return_value=[]), \
             mock.patch(NAMESPACE + '.CoreCpuReduceViewer._get_ai_cpu_data', return_value=[]), \
             mock.patch(NAMESPACE + '.CoreCpuReduceViewer._get_all_reduce_data', return_value=[]):
            res = CoreCpuReduceViewer.get_task_time_data(params)
        self.assertEqual(res, "[]")

# if __name__ == '__main__':
#    unittest.main()

import unittest
from unittest import mock

from common_func.db_name_constant import DBNameConstant
from sqlite.db_manager import DBManager
from sqlite.db_manager import DBOpen
from viewer.training.core_cpu_reduce_viewer import CoreCpuReduceViewer

NAMESPACE = 'viewer.training.core_cpu_reduce_viewer'


class TestCoreCpuReduceViewer(unittest.TestCase):
    def setUp(self):
        _db_manager = DBManager()
        _db_manager.destroy(_db_manager.create_sql(DBNameConstant.DB_TRACE))

    def tearDown(self):
        _db_manager = DBManager()
        _db_manager.destroy(_db_manager.create_sql(DBNameConstant.DB_TRACE))

    def test_get_rts_track_task_type(self):
        result = {}
        create_sql = "CREATE TABLE IF NOT EXISTS " + DBNameConstant.TABLE_RUNTIME_TRACK + \
                     "(device_id,timestamp,task_type,stream_id,task_id,thread,batch_id)"
        data = ((0, 3450, 'MEMCPY_ASYNC', 2, 1, 1211, 0),)
        with DBOpen(DBNameConstant.DB_RTS_TRACK) as db_open:
            db_open.create_table(create_sql)
            db_open.insert_data(DBNameConstant.TABLE_RUNTIME_TRACK, data)
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path',
                            return_value=(db_open.db_conn, db_open.db_curs)), \
                    mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                    mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
                res = CoreCpuReduceViewer.get_rts_track_task_type("")
            self.assertEqual(res, result)

    def test_get_task_type_from_stars(self):
        result = {'1_1_1': 'test'}
        create_sql = "CREATE TABLE IF NOT EXISTS " + DBNameConstant.TABLE_SUMMARY_TASK_TIME + \
                     "(device_id,timestamp,task_type,stream_id,task_id,thread,batch_id)"
        data = ((0, 3450, 'MEMCPY_ASYNC', 2, 1, 1211, 0),)
        with DBOpen(DBNameConstant.DB_AICORE_OP_SUMMARY) as db_open:
            db_open.create_table(create_sql)
            db_open.insert_data(DBNameConstant.TABLE_SUMMARY_TASK_TIME, data)
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path',
                            return_value=(db_open.db_conn, db_open.db_curs)), \
                    mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                    mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'), \
                    mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[('test', 1, 1, 1)]):
                res = CoreCpuReduceViewer.get_task_type_from_stars("")
            self.assertEqual(res, result)

    def test_get_op_names_and_task_type_1(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(True, True)), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            res = CoreCpuReduceViewer.get_op_names_and_task_type('')
        self.assertEqual(res[0], {})

    def test_get_op_names_and_task_type_2(self):
        ge_data = [("a", 3, 2, "AI_CORE", 1)]
        result = {"3_2_1": "a"}
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(True, True)), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=ge_data), \
                mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            res = CoreCpuReduceViewer.get_op_names_and_task_type('')
        self.assertEqual(result, res[0])

    def test_get_total_cycle_1(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(True, True)), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            res = CoreCpuReduceViewer.get_total_cycle('')
        self.assertEqual(res[0], dict())

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(True, True)), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            res = CoreCpuReduceViewer.get_total_cycle('')
        self.assertEqual(res[0], dict())

    def test_get_total_cycle_2(self):
        result = {'12_2_0': 3450}
        create_sql = "CREATE TABLE IF NOT EXISTS " + DBNameConstant.TABLE_AI_CORE_METRIC_SUMMARY + \
                     "(total_cycles, stream_id, task_id, index_id, total_time)"
        data = ((3450, 12, 2, 1, 0.00345),)
        with DBOpen(DBNameConstant.DB_RUNTIME) as db_open:
            db_open.create_table(create_sql)
            db_open.insert_data(DBNameConstant.TABLE_AI_CORE_METRIC_SUMMARY, data)
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path',
                            return_value=(db_open.db_conn, db_open.db_curs)), \
                    mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                    mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
                res = CoreCpuReduceViewer.get_total_cycle("")
            self.assertEqual(res[0], result)

    def test_get_task_time_data(self):
        params = {"device_id": 0,
                  "iter_id": 1,
                  "result_dir": "result_dir"}
        with mock.patch(NAMESPACE + '.CoreCpuReduceViewer._get_task_scheduler_data', return_value=[]), \
                mock.patch(NAMESPACE + '.CoreCpuReduceViewer._get_ai_cpu_data', return_value=[]), \
                mock.patch(NAMESPACE + '.CoreCpuReduceViewer._get_all_reduce_data', return_value=[]):
            res = CoreCpuReduceViewer.get_task_time_data(params)
        self.assertEqual(res, "")

    def test_get_task_time_data_1(self):
        params = {"device_id": 0,
                  "iter_id": 1,
                  "result_dir": "result_dir"}
        data = ((0, 31, 17038931452445, 0, 'PLACE_HOLDER_SQE', 1, 10, 0),
                (0, 22, 17038931496665, 1267120, 'AI_CPU', 1, 10, 0))
        with mock.patch("msmodel.interface.view_model.ViewModel.get_all_data", return_value=data):
            res = CoreCpuReduceViewer.get_task_time_data(params)
        self.assertEqual(res, "")

    def test_get_task_scheduler_data_1(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(True, True)), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            res = CoreCpuReduceViewer._get_task_scheduler_data("")
        self.assertEqual(res, [])

    def test_get_task_scheduler_data_2(self):
        result = ["time_graph_trace", "meta_trace_data"]
        create_sql = "CREATE TABLE IF NOT EXISTS " + DBNameConstant.TABLE_SUMMARY_TASK_TIME + \
                     "(stream_id,task_id,start_time,duration_time,task_type,index_id,batch_id)"
        task_scheduler_data = ((13, 3, 518071658150960, 10, 'AI_CORE', 1, 0),)
        with DBOpen(DBNameConstant.DB_AICORE_OP_SUMMARY) as db_open:
            db_open.create_table(create_sql)
            db_open.insert_data(DBNameConstant.TABLE_SUMMARY_TASK_TIME, task_scheduler_data)
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path',
                            return_value=(db_open.db_conn, db_open.db_curs)), \
                 mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                 mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'), \
                 mock.patch(NAMESPACE + '.DBManager.fetch_all_data'), \
                 mock.patch(NAMESPACE + '.TraceViewManager.time_graph_trace', return_value=["time_graph_trace"]), \
                 mock.patch(NAMESPACE + '.CoreCpuReduceViewer.get_meta_trace_data', return_value=["meta_trace_data"]):
                res = CoreCpuReduceViewer._get_task_scheduler_data("")
        self.assertEqual(res, result)

    def test_get_ai_cpu_data(self):
        result = ["time_graph_trace", "meta_trace_data"]
        with mock.patch(NAMESPACE + '.ParseAiCpuData.get_ai_cpu_from_ts', return_value=[]):
            res = CoreCpuReduceViewer._get_ai_cpu_data("")
        self.assertEqual(res, [])

        with mock.patch(NAMESPACE + '.ParseAiCpuData.get_ai_cpu_from_ts'), \
             mock.patch(NAMESPACE + '.TraceViewManager.time_graph_trace', return_value=[]):
            res = CoreCpuReduceViewer._get_ai_cpu_data("")
        self.assertEqual(res, [])

        with mock.patch(NAMESPACE + '.ParseAiCpuData.get_ai_cpu_from_ts'), \
             mock.patch(NAMESPACE + '.TraceViewManager.time_graph_trace', return_value=["time_graph_trace"]), \
             mock.patch(NAMESPACE + '.CoreCpuReduceViewer.get_meta_trace_data', return_value=["meta_trace_data"]):
            res = CoreCpuReduceViewer._get_ai_cpu_data("")
        self.assertEqual(res, result)

    def test_get_all_reduce_data(self):
        result = ["time_graph_trace", "meta_trace_data"]
        from profiling_bean.db_dto.step_trace_dto import IterationRange
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(False, False)):
            res = CoreCpuReduceViewer._get_all_reduce_data("0", IterationRange(1, 2, 3), "")
        self.assertEqual(res, [])

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(True, True)), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            res = CoreCpuReduceViewer._get_all_reduce_data("0", IterationRange(1, 2, 3), "")
        self.assertEqual(res, [])

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(True, True)), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch(NAMESPACE + '.CoreCpuReduceViewer._format_all_reduce_data', return_value=[]), \
                mock.patch(NAMESPACE + '.TraceViewManager.time_graph_trace', return_value=[]):
            res = CoreCpuReduceViewer._get_all_reduce_data("0", IterationRange(1, 2, 3), "")
        self.assertEqual(res, [])

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(True, True)), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
             mock.patch(NAMESPACE + '.CoreCpuReduceViewer._format_all_reduce_data', return_value=[]), \
             mock.patch(NAMESPACE + '.TraceViewManager.time_graph_trace', return_value=["time_graph_trace"]), \
             mock.patch(NAMESPACE + '.CoreCpuReduceViewer.get_meta_trace_data', return_value=["meta_trace_data"]):
            res = CoreCpuReduceViewer._get_all_reduce_data("0", IterationRange(1, 2, 3), "")
        self.assertEqual(res, result)

if __name__ == '__main__':
    unittest.main()

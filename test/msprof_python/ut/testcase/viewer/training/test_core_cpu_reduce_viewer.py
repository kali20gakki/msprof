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
        result = {'2_1_0': 'MEMCPY_ASYNC'}
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
        result = {'12_2_0': 12}
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
        self.assertEqual(res, '[{"name": "process_name", "pid": 3, "tid": 0, "args": {"name": "ACSQ"}, "ph": "M"}]')


if __name__ == '__main__':
    unittest.main()

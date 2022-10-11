import sqlite3
import unittest
import copy
from unittest import mock

from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from sqlite.db_manager import DBManager
from viewer.training.task_op_viewer import TaskOpViewer

NAMESPACE = 'viewer.training.task_op_viewer'
message = {"result_dir": '',
           "device_id": "4",
           "sql_path": ''}


class TestTaskOpViewer(unittest.TestCase):

    def test_get_task_op_summary_2(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
                mock.patch(NAMESPACE + '.TaskOpViewer.get_op_task_data_summary', return_value=([], 0)), \
                mock.patch(NAMESPACE + '.TaskOpViewer._add_memcpy_data', return_value=[]):
            ProfilingScene()._scene = Constant.SINGLE_OP
            res = TaskOpViewer.get_task_op_summary(message)
        self.assertEqual(res[2], 0)

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
                mock.patch(NAMESPACE + '.TaskOpViewer.get_op_task_data_summary', return_value=([], 0)), \
                mock.patch(NAMESPACE + '.TaskOpViewer._add_memcpy_data', return_value=[]):
            check = TaskOpViewer()
            ProfilingScene().init('')
            ProfilingScene()._scene = Constant.STEP_INFO
        res = check.get_task_op_summary(message)
        self.assertEqual(res[2], 0)

    def test_get_task_op_summary_3(self):
        op_task_data_summary = [('trans_TransData_0', 'AI_CORE', 148, 3, '"7.62"',
                                 '"155149754006240"', '"155149754013860"'),
                                ('trans_TransData_2', 'AI_CORE', 148, 6, '"2.01"',
                                 '"155151161417390"', '"155151161419400"')]
        task_data_summary = copy.deepcopy(op_task_data_summary)
        count = 2

        expect_res = (["kernel_name", "kernel_type", "stream_id", "task_id",
                   "task_time(us)", "task_start(ns)", "task_stop(ns)"],
            [('trans_TransData_0', 'AI_CORE', 148, 3, '"7.62"',
                        '"155149754006240"', '"155149754013860"'),
                        ('trans_TransData_2', 'AI_CORE', 148, 6, '"2.01"',
                        '"155151161417390"', '"155151161419400"'),
                        ("MemcopyAsync", "other", 11, 12, "100", "2200", "2300")],
                      3)

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
                mock.patch(NAMESPACE + '.TaskOpViewer.get_op_task_data_summary',
                           return_value=(op_task_data_summary, count)), \
                mock.patch('viewer.memory_copy.memory_copy_viewer.'
                           'MemoryCopyViewer.get_memory_copy_non_chip0_summary',
                           return_value=[("MemcopyAsync", "other", 11, 12, "100", "2200", "2300")]):
            ProfilingScene()._scene = Constant.SINGLE_OP
            res = TaskOpViewer.get_task_op_summary(message)

        self.assertEqual(expect_res, res)

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
                mock.patch(NAMESPACE + '.TaskOpViewer.get_task_data_summary',
                           return_value=(task_data_summary, count)), \
                mock.patch('viewer.memory_copy.memory_copy_viewer.'
                           'MemoryCopyViewer.get_memory_copy_non_chip0_summary',
                           return_value=[("MemcopyAsync", "other", 11, 12, "100", "2200", "2300")]):
            ProfilingScene()._scene = Constant.STEP_INFO
            res = TaskOpViewer.get_task_op_summary(message)

        self.assertEqual(expect_res, res)

    def test_get_task_data_summary_1(self):
        res = TaskOpViewer.get_task_data_summary(None, None, None)
        self.assertEqual(res[1], 0)

        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False), \
                mock.patch(NAMESPACE + '.logging.warning'):
            res = TaskOpViewer.get_task_data_summary(1, 2, 3)
        self.assertEqual(res[1], 0)

        with mock.patch(NAMESPACE + '.TaskOpViewer._reformat_task_info', side_effect=TypeError), \
                mock.patch(NAMESPACE + '.logging.error'):
            res = TaskOpViewer.get_task_data_summary(1, 2, 3)
        self.assertEqual(res[1], 0)

    def test_get_task_data_summary_2(self):
        db_manager_hwts = DBManager()
        db_manager_rts = DBManager()
        db_manager_ge = DBManager()

        create_hwts_sql = "CREATE TABLE IF NOT EXISTS " + DBNameConstant.TABLE_HWTS_TASK_TIME + \
                          "(device_id, stream_id, task_id, running, complete, index_id, batch_id)"
        data = ((4, 0, 3, 18870605628550, 18870605628550, 1, 0),)
        insert_hwts_sql = "insert into {0} values ({value})".format(
            DBNameConstant.TABLE_HWTS_TASK_TIME, value="?," * (len(data[0]) - 1) + "?")
        test_hwts_sql = db_manager_hwts.create_table(DBNameConstant.DB_HWTS, create_hwts_sql, insert_hwts_sql, data)

        create_rts_sql = "CREATE TABLE IF NOT EXISTS " + DBNameConstant.TABLE_RUNTIME_TRACK + \
                         "(device_id, timestamp, tasktype, stream_id, task_id, " \
                         "batch_id, thread)"
        data = ((0, 18870603856900, "TaskSubmited", 0, 3, 0, 3988),
                (0, 18870603856900, "TaskSubmited", 1, 2, 0, 3988),)
        insert_rts_sql = "insert into {0} values ({value})".format(
            DBNameConstant.TABLE_RUNTIME_TRACK, value="?," * (len(data[0]) - 1) + "?")
        test_rts_sql = db_manager_rts.create_table(DBNameConstant.DB_RTS_TRACK, create_rts_sql, insert_rts_sql, data)

        create_ge_sql = "CREATE TABLE {0}(model_id INTEGER,op_name TEXT," \
                        "stream_id INTEGER,task_id INTEGER,block_dim INTEGER,op_state TEXT,task_type TEXT," \
                        "op_type TEXT,index_id INTEGER,thread_id INTEGER,timestamp INTEGER, batch_id INTEGER)".format(
            DBNameConstant.TABLE_GE_TASK)
        data = ((1, "AI CORE NAME", 0, 3, 4, "0", "static", "cast", 0, 122, 123456, 0),
                (1, "AI CPU NAME", 1, 2, 4, "0", "static", "cast", 0, 122, 123456, 0))
        insert_ge_sql = "insert into {0} values ({value})".format(
            DBNameConstant.TABLE_GE_TASK, value="?," * (len(data[0]) - 1) + "?")
        test_ge_sql = db_manager_ge.create_table(DBNameConstant.DB_GE_INFO, create_ge_sql, insert_ge_sql, data)

        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch(NAMESPACE + '.logging.error'):
            res = TaskOpViewer.get_task_data_summary(test_hwts_sql[1], test_rts_sql[1], test_ge_sql[1])
        (test_hwts_sql[1]).execute("drop Table HwtsTaskTime")
        db_manager_hwts.destroy(test_hwts_sql)
        (test_rts_sql[1]).execute("drop Table RuntimeTrack")
        db_manager_rts.destroy(test_rts_sql)
        db_manager_ge.destroy(test_ge_sql)
        self.assertEqual(res[1], 1)

    def test_get_op_task_data_summary_1(self):
        with mock.patch(NAMESPACE + '.logging.warning'):
            res = TaskOpViewer.get_op_task_data_summary(None, None)
        self.assertEqual(res[1], 0)

        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False), \
                mock.patch(NAMESPACE + '.logging.warning'):
            res = TaskOpViewer.get_op_task_data_summary(1, 2)
        self.assertEqual(res[1], 0)

        with mock.patch(NAMESPACE + '.TaskOpViewer._reformat_op_task_info', side_effect=TypeError), \
                mock.patch(NAMESPACE + '.logging.error'):
            res = TaskOpViewer.get_op_task_data_summary(1, 2)
        self.assertEqual(res[1], 0)

    def test_get_op_task_data_summary_2(self):
        create_hwts_sql = "CREATE TABLE IF NOT EXISTS " + DBNameConstant.TABLE_HWTS_TASK_TIME + \
                          "(device_id, stream_id, task_id, running, complete, index_id, batch_id)"
        data = ((4, 0, 3, 18870605628550, 18870605628550, 1, 0),)
        insert_hwts_sql = "insert into {0} values ({value})".format(
            DBNameConstant.TABLE_HWTS_TASK_TIME, value="?," * (len(data[0]) - 1) + "?")
        db_manager_hwts = DBManager()
        db_manager_ge = DBManager()
        test_hwts_sql = db_manager_hwts.create_table(DBNameConstant.DB_HWTS, create_hwts_sql, insert_hwts_sql, data)

        create_ge_sql = "CREATE TABLE IF NOT EXISTS " + DBNameConstant.TABLE_GE_TASK + \
                        "(device_id, model_name, model_id, op_name, stream_id, task_id, block_dim, op_state, task_type," \
                        " op_type, iter_id, input_count, input_formats, input_data_types, input_shapes, output_count," \
                        " output_formats, output_data_types, output_shapes, batch_id)"
        data = ((4, "ge_default_20210326080919_1", 1, "AI CORE NAME", 0, 3, 1, "static", "AI_CORE", "Assign",
                 0, 2, "ND;ND", "DT_INT64;DT_INT64", ";", 1, "ND", "DT_INT64", "", 0),
                (4, "ge_default_20210326080919_1", 1, "AICPU NODE", 1, 2, 1, "static", "AI_CPU", "Assign",
                 0, 2, "ND;ND", "DT_INT64;DT_INT64", ";", 1, "ND", "DT_INT64", "", 0),)
        insert_ge_sql = "insert into {0} values ({value})".format(
            DBNameConstant.TABLE_GE_TASK, value="?," * (len(data[0]) - 1) + "?")
        test_ge_sql = db_manager_ge.create_table(DBNameConstant.DB_GE_INFO, create_ge_sql, insert_ge_sql, data)

        expect_res = [("AI CORE NAME", "AI_CORE", 0, 3, '0.0', '\"18870605628550\"', '\"18870605628550\"')]
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch(NAMESPACE + '.logging.error'):
            res = TaskOpViewer.get_op_task_data_summary(test_hwts_sql[1], test_ge_sql[1])
        (test_hwts_sql[1]).execute("drop Table HwtsTaskTime")
        db_manager_hwts.destroy(test_hwts_sql)
        db_manager_ge.destroy(test_ge_sql)
        self.assertEqual(res[0], expect_res)

    def test_reformat_op_task_info(self):
        expect_res = [("AI CORE NAME", "AI_CORE", 0, 3, '0.0', '\"18870605628550\"', '\"18870605628550\"'),
                      ("AICPU NODE", "AI_CPU", 1, 2, '0.01', '\"10\"', '\"20\"')]
        task_data = [[0, 3, 18870605628550, 18870605628550, 0], [1, 2, 10, 20, 0]]
        ge_data = [[0, 3, "AI CORE NAME", "AI_CORE", 0], [1, 2, "AICPU NODE", "AI_CPU", 0]]
        task_info = TaskOpViewer._reformat_op_task_info(task_data, ge_data)
        self.assertEqual(expect_res, task_info)


if __name__ == '__main__':
    unittest.main()

import unittest
from unittest import mock

from common_func.db_name_constant import DBNameConstant
from sqlite.db_manager import DBManager
from viewer.training.task_op_viewer import TaskOpViewer

NAMESPACE = 'viewer.training.task_op_viewer'
message = {
    "result_dir": '',
    "device_id": "4",
    "sql_path": '',
}


class TestTaskOpViewer(unittest.TestCase):

    def test_get_task_op_summary_empty(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
                mock.patch(NAMESPACE + '.TaskOpViewer.get_task_data_summary', return_value=([], 0)), \
                mock.patch(NAMESPACE + '.TaskOpViewer._add_memcpy_data', return_value=[]):
            _, _, count = TaskOpViewer.get_task_op_summary(message)
            self.assertEqual(count, 0)

    def test_get_task_op_summary_should_match_2aicore_and_1memcpy(self):
        task_data_summary = [
            ('trans_TransData_0', 'AI_CORE', 148, 3, '"7.62"', '"155149754006240"', '"155149754013860"'),
            ('trans_TransData_2', 'AI_CORE', 148, 6, '"2.01"', '"155151161417390"', '"155151161419400"'),
        ]

        expect_headers = [
            "kernel_name", "kernel_type", "stream_id", "task_id",
            "task_time(us)", "task_start(ns)", "task_stop(ns)",
        ]
        expect_data = [
            ('trans_TransData_0', 'AI_CORE', 148, 3, '"7.62"', '"155149754006240"', '"155149754013860"'),
            ('trans_TransData_2', 'AI_CORE', 148, 6, '"2.01"', '"155151161417390"', '"155151161419400"'),
            ("MemcopyAsync", "other", 11, 12, "100", "2200", "2300"),
        ]

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
                mock.patch(NAMESPACE + '.TaskOpViewer.get_task_data_summary',
                           return_value=(task_data_summary, len(task_data_summary))), \
                mock.patch('viewer.memory_copy.memory_copy_viewer.'
                           'MemoryCopyViewer.get_memory_copy_non_chip0_summary',
                           return_value=[("MemcopyAsync", "other", 11, 12, "100", "2200", "2300")]):
            headers, data, count = TaskOpViewer.get_task_op_summary(message)
            self.assertEqual(expect_headers, headers)
            self.assertEqual(len(expect_data), count)
            self.assertEqual(expect_data, data)

    def test_get_task_data_summary_should_fail(self):
        res = TaskOpViewer.get_task_data_summary(None, None)
        self.assertEqual(res[1], 0)

        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False), \
                mock.patch(NAMESPACE + '.logging.warning'):
            _, count = TaskOpViewer.get_task_data_summary(1, 2)
            self.assertEqual(count, 0)

        with mock.patch(NAMESPACE + '.TaskOpViewer._reformat_task_info', side_effect=TypeError), \
                mock.patch(NAMESPACE + '.logging.error'):
            _, count = TaskOpViewer.get_task_data_summary(1, 2)
            self.assertEqual(count, 0)

    def test_get_task_data_summary_should_match_cast_and_update_opname(self):
        db_manager_ascend = DBManager()
        db_manager_ge = DBManager()

        create_ascend_sql = "CREATE TABLE IF NOT EXISTS " +\
            DBNameConstant.TABLE_ASCEND_TASK + \
            "(model_id, index_id, stream_id, task_id, context_id, batch_id" \
            ", start_time, duration, host_task_type, device_task_type)"

        ascend_data = (
            (1, 0, 4, 0, 4294967295, 0, 18870605628550, 2310, "AICORE", "AICORE"),
            (1, 0, 4, 1, 4294967295, 0, 18870605628870, 35.5, "AICORE", "AICORE"),
        )
        expect_data = [
            ('N/A', 'AICORE', 4, 0, 2.310, '"18870605628550"', '"18870605630860"'),
            ('Cast', 'AICORE', 4, 1, 0.0355, '"18870605628870"', '"18870605628905.5"'),
        ]

        insert_ascend_sql = "insert into {0} values ({value})".format(
            DBNameConstant.TABLE_ASCEND_TASK, value="?," * (len(ascend_data[0]) - 1) + "?")
        ascend_conn, ascend_cur = db_manager_ascend.create_table(
            DBNameConstant.DB_ASCEND_TASK, create_ascend_sql, insert_ascend_sql, ascend_data)

        create_ge_sql = "CREATE TABLE {0}(model_id INTEGER,op_name TEXT," \
                        "stream_id INTEGER,task_id INTEGER,block_dim INTEGER,op_state TEXT,task_type TEXT," \
                        "op_type TEXT,index_id INTEGER,thread_id INTEGER" \
                        ",timestamp INTEGER, batch_id INTEGER, context_id INTEGER)".format(
                            DBNameConstant.TABLE_GE_TASK)
        ge_data = (
            (1, "AI CORE NAME", 0, 3, 4, "0", "static", "cast", 0, 122, 123456, 0, 4294967295),
            (1, "AI CPU NAME", 1, 2, 4, "0", "static", "cast", 0, 122, 123456, 0, 4294967295),
            (1, "Cast", 4, 1, 32, "0", "static", "cast", 0, 122, 123456, 0, 4294967295),
        )
        insert_ge_sql = "insert into {0} values ({value})".format(
            DBNameConstant.TABLE_GE_TASK, value="?," * (len(ge_data[0]) - 1) + "?")
        ge_conn, ge_cur = db_manager_ge.create_table(DBNameConstant.DB_GE_INFO, create_ge_sql, insert_ge_sql, ge_data)

        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch(NAMESPACE + '.logging.error'):
            res, count = TaskOpViewer.get_task_data_summary(ascend_cur, ge_cur)
            self.assertEqual(count, len(expect_data))
            self.assertEqual(expect_data, res)

        ascend_cur.execute("drop Table " + DBNameConstant.TABLE_ASCEND_TASK)
        ge_cur.execute("drop Table " + DBNameConstant.TABLE_GE_TASK)
        db_manager_ascend.destroy((ascend_conn, ascend_cur))
        db_manager_ge.destroy((ge_conn, ge_cur))


if __name__ == '__main__':
    unittest.main()

import sqlite3
from unittest import mock

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from model.test_dir_cr_base_model import TestDirCRBaseModel
from msmodel.stars.op_summary_model import OpSummaryModel
from sqlite.db_manager import DBOpen

NAMESPACE = 'msmodel.stars.op_summary_model'
SAMPLE_CONFIG = {"result_dir": "test",
                 "iter_id": 1,
                 "model_id": 1}


class TestOpSummaryModel(TestDirCRBaseModel):

    def test_create_table(self):
        with mock.patch(NAMESPACE + '.OpSummaryModel.create_ge_summary_table', return_value=""), \
             mock.patch(NAMESPACE + '.OpSummaryModel.create_ge_tensor_table', return_value=""), \
             mock.patch(NAMESPACE + '.OpSummaryModel.create_ai_core_metrics_table', return_value=""), \
             mock.patch(NAMESPACE + '.OpSummaryModel.create_task_time_table', return_value=""), \
             mock.patch(NAMESPACE + '.OpSummaryModel.sql_commit', return_value=""):
            check = OpSummaryModel(SAMPLE_CONFIG)
            check.create_table()

    def test_create_ge_summary_table(self):
        with mock.patch(NAMESPACE + '.OpSummaryModel.attach_to_db', return_value=True), \
             mock.patch(NAMESPACE + '.DBManager.execute_sql'):
            check = OpSummaryModel(SAMPLE_CONFIG)
            check.create_ge_summary_table()

        with mock.patch(NAMESPACE + '.OpSummaryModel.attach_to_db', return_value=False), \
             mock.patch(NAMESPACE + '.logging.warning'):
            check = OpSummaryModel(SAMPLE_CONFIG)
            check.create_ge_summary_table()

    def test_create_ge_tensor_table(self):
        with mock.patch(NAMESPACE + '.DBManager.execute_sql'):
            check = OpSummaryModel(SAMPLE_CONFIG)
            check.create_ge_tensor_table()

    def test_create_ai_core_metrics_table(self):
        with mock.patch(NAMESPACE + '.OpSummaryModel.attach_to_db', return_value=True), \
             mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True), \
             mock.patch(NAMESPACE + '.DBManager.execute_sql'):
            check = OpSummaryModel(SAMPLE_CONFIG)
            check.create_ai_core_metrics_table()

        with mock.patch(NAMESPACE + '.OpSummaryModel.attach_to_db', return_value=False), \
             mock.patch(NAMESPACE + '.logging.warning'):
            check = OpSummaryModel(SAMPLE_CONFIG)
            check.create_ai_core_metrics_table()

        with mock.patch(NAMESPACE + '.OpSummaryModel.attach_to_db', return_value=True), \
             mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=False), \
             mock.patch(NAMESPACE + '.logging.warning'):
            check = OpSummaryModel(SAMPLE_CONFIG)
            check.create_ai_core_metrics_table()

    def test_create_task_time_table(self):
        with mock.patch(NAMESPACE + '.DBManager.sql_create_general_table', return_value="TEST"), \
             mock.patch(NAMESPACE + '.OpSummaryModel.get_task_time_data', return_value=[[1]]), \
             mock.patch(NAMESPACE + '.OpSummaryModel.insert_data_to_db'), \
             mock.patch(NAMESPACE + '.DBManager.execute_sql'):
            check = OpSummaryModel(SAMPLE_CONFIG)
            check.create_task_time_table()

        with mock.patch(NAMESPACE + '.DBManager.sql_create_general_table', return_value=""), \
             mock.patch(NAMESPACE + '.logging.error'):
            check = OpSummaryModel(SAMPLE_CONFIG)
            check.create_task_time_table()

        with mock.patch(NAMESPACE + '.DBManager.sql_create_general_table', return_value="TEST"), \
             mock.patch(NAMESPACE + '.OpSummaryModel.get_task_time_data', return_value=[]), \
             mock.patch(NAMESPACE + '.logging.warning'), \
             mock.patch(NAMESPACE + '.DBManager.execute_sql'):
            check = OpSummaryModel(SAMPLE_CONFIG)
            check.create_task_time_table()

    def test_get_task_time_data(self):
        with mock.patch(NAMESPACE + '.OpSummaryModel._get_ffts_task_data', return_value=[]), \
             mock.patch(NAMESPACE + '.OpSummaryModel._get_acsq_task_data', return_value=[1]):
            check = OpSummaryModel(SAMPLE_CONFIG)
            res = check.get_task_time_data()
        self.assertEqual(res, [[1]])

    def test_get_acsq_task_data(self):
        with mock.patch(NAMESPACE + '.logging.error'), \
             mock.patch(NAMESPACE + '.AcsqTaskModel.get_ffts_type_data', side_effect=sqlite3.DatabaseError):
            check = OpSummaryModel(SAMPLE_CONFIG)
            res = check._get_acsq_task_data()
        self.assertEqual(res, [])

        with mock.patch(NAMESPACE + '.AcsqTaskModel.get_ffts_type_data', return_value=[1]):
            check = OpSummaryModel(SAMPLE_CONFIG)
            res = check._get_acsq_task_data()
        self.assertEqual(res, [1])

    def test_get_ffts_task_data(self):
        with mock.patch(NAMESPACE + '.logging.error'), \
             mock.patch(NAMESPACE + '.FftsLogModel.get_ffts_task_data', side_effect=sqlite3.DatabaseError):
            check = OpSummaryModel(SAMPLE_CONFIG)
            res = check._get_ffts_task_data()
        self.assertEqual(res, [])

        with mock.patch(NAMESPACE + '.FftsLogModel.get_ffts_task_data', return_value=[1]):
            check = OpSummaryModel(SAMPLE_CONFIG)
            res = check._get_ffts_task_data()
        self.assertEqual(res, [1])

    def test_get_db_path(self):
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value="test"):
            check = OpSummaryModel(SAMPLE_CONFIG)
            res = check.get_db_path("test")
        self.assertEqual(res, "test")

    def test_get_operator_data_by_task_type_should_return_different_data_when_is_empty(self):
        create_ge_summary_sql = "create table IF NOT EXISTS {0} " \
                                "(model_id INTEGER, task_id INTEGER, stream_id INTEGER," \
                                "op_name TEXT, op_type TEXT, block_dim INTEGER, mix_block_dim INTEGER," \
                                "task_type TEXT, timestamp INTEGER,  batch_id INTEGER," \
                                "context_id INTEGER)".format(DBNameConstant.TABLE_SUMMARY_GE)
        create_task_time_sql = "create table IF NOT EXISTS {0} " \
                               "(task_id INTEGER, stream_id INTEGER, start_time INTEGER, duration_time INTEGER," \
                               "wait_time TEXT, task_type TEXT, index_id INTEGER, model_id INTEGER," \
                               "batch_id TEXT, subtask_id INTEGER)".format(DBNameConstant.TABLE_SUMMARY_TASK_TIME)

        ge_summary_data = [
            (4294967295, 1, 23, "ai_core", "ai_core", 1, 0, "AI_CORE", 397531459726750, 0, 4294967295),
            (4294967295, 2, 23, "ai_core", "ai_core", 1, 0, "AI_CORE", 397531459726751, 0, 4294967295),
            (4294967295, 3, 23, "ai_cpu", "ai_cpu", 1, 0, "AI_CPU", 397531459726752, 0, 4294967295),
            (4294967295, 4, 23, "ai_vector", "ai_vector", 1, 0, "AI_VECTOR_CORE", 397531459726753, 0, 4294967295),
            (4294967295, 5, 23, "mix_aiv", "mix_aiv", 1, 0, "MIX_AIV", 397531459726754, 0, 4294967295),
            (4294967295, 6, 23, "mix_aic", "mix_aic", 1, 0, "MIX_AIC", 397531459726755, 0, 4294967295),
        ]

        task_time_data = [
            (1, 23, 397531459795910, 4280, 0, "AI_CORE", -1, 4294967295, 0, 4294967295),
            (2, 23, 397531459795911, 4280, 0, "UNKNOWN", -1, 4294967295, 0, 4294967295),
            (3, 23, 397531459795912, 4280, 0, "AI_CPU", -1, 4294967295, 0, 4294967295),
            (4, 23, 397531459795913, 4280, 0, "AI_VECTOR_CORE", -1, 4294967295, 0, 4294967295),
            (5, 23, 397531459795914, 4280, 0, "MIX_AIV", -1, 4294967295, 0, 4294967295),
            (6, 23, 397531459795915, 4280, 0, "MIX_AIC", -1, 4294967295, 0, 4294967295),
        ]

        with DBOpen(DBNameConstant.DB_AICORE_OP_SUMMARY) as db_open, \
             mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value=db_open.db_path):
            db_open.create_table(create_ge_summary_sql)
            db_open.create_table(create_task_time_sql)
            db_open.insert_data(DBNameConstant.TABLE_SUMMARY_GE, ge_summary_data)
            db_open.insert_data(DBNameConstant.TABLE_SUMMARY_TASK_TIME, task_time_data)
            check = OpSummaryModel(SAMPLE_CONFIG)
            check.cur = db_open.db_curs
            check.conn = db_open.db_conn
            res = check.get_operator_data_by_task_type()
            self.assertEqual(len(res), 5)

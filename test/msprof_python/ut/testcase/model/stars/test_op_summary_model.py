import sqlite3
import unittest
from unittest import mock

from msmodel.stars.op_summary_model import OpSummaryModel

NAMESPACE = 'msmodel.stars.op_summary_model'
SAMPLE_CONFIG = {"result_dir": "test",
                 "iter_id": 1,
                 "model_id": 1}


class TestOpSummaryModel(unittest.TestCase):

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

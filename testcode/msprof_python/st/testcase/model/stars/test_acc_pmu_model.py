import sqlite3
import unittest
from unittest import mock

from model.stars.acc_pmu_model import AccPmuModel

NAMESPACE = 'model.stars.acc_pmu_model'


class TestAccPmuModel(unittest.TestCase):

    def test_create_table(self):
        with mock.patch(NAMESPACE + '.DBManager.sql_create_general_table', return_value=""), \
             mock.patch(NAMESPACE + '.DBManager.execute_sql'):
            check = AccPmuModel('test', 'test', [])
            check.create_table()

    def test_insert_pmu_data(self):
        with mock.patch(NAMESPACE + '.AccPmuModel._AccPmuModel__get_task_time_form_acsq', return_value=[]), \
             mock.patch(NAMESPACE + '.Utils.generator_to_list', return_value=[[0, 1, 2]]), \
             mock.patch(NAMESPACE + '.DBManager.executemany_sql'):
            check = AccPmuModel('test', 'test', [])
            check.insert_pmu_data([])

    def test_flush(self):
        with mock.patch(NAMESPACE + '.AccPmuModel.insert_pmu_data'):
            check = AccPmuModel('test', 'test', [])
            check.flush([])

    def test_get_timeline_data(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
             mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[1]):
            check = AccPmuModel('test', 'test', [])
            res = check.get_timeline_data()
        self.assertEqual(res, [1])

        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            check = AccPmuModel('test', 'test', [])
            res = check.get_timeline_data()
        self.assertEqual(res, [])

    def test_get_summary_data(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
             mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[1]):
            check = AccPmuModel('test', 'test', [])
            res = check.get_summary_data()
        self.assertEqual(res, [1])

        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            check = AccPmuModel('test', 'test', [])
            res = check.get_summary_data()
        self.assertEqual(res, [])

    def test_get_task_time_form_acsq(self):
        with mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'), \
             mock.patch(NAMESPACE + '.DBManager.create_connect_db', return_value=(True, True)), \
             mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[[1, 2, 3]]):
            check = AccPmuModel('test', 'test', [])
            res = check._AccPmuModel__get_task_time_form_acsq()
        self.assertEqual(res, {1: (2, 3)})

    def test_get_task_time_form_acsq_err(self):
        with mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'), \
             mock.patch(NAMESPACE + '.DBManager.create_connect_db', return_value=(None, None)):
            check = AccPmuModel('test', 'test', [])
            res = check._AccPmuModel__get_task_time_form_acsq()
        self.assertEqual(res, {})

        with mock.patch(NAMESPACE + '.DBManager.create_connect_db', return_value=(True, True)), \
             mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'), \
             mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[[]]):
            check = AccPmuModel('test', 'test', [])
            res = check._AccPmuModel__get_task_time_form_acsq()
        self.assertEqual(res, {})

        with mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'), \
             mock.patch(NAMESPACE + '.DBManager.create_connect_db', return_value=(True, True)), \
             mock.patch(NAMESPACE + '.logging.error'), \
             mock.patch(NAMESPACE + '.DBManager.fetch_all_data', side_effect=sqlite3.DatabaseError):
            check = AccPmuModel('test', 'test', [])
            res = check._AccPmuModel__get_task_time_form_acsq()
        self.assertEqual(res, {})

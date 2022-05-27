import sqlite3
import unittest
from unittest import mock

from model.stars.acc_pmu_model import AccPmuModel

NAMESPACE = 'model.stars.acc_pmu_model'


class TestAccPmuModel(unittest.TestCase):

    def test_insert_pmu_origin_data(self):
        with mock.patch(NAMESPACE + '.AccPmuModel.insert_data_to_db'):
            check = AccPmuModel('test', 'test', [])
            check.insert_pmu_origin_data([])

    def test_flush(self):
        with mock.patch(NAMESPACE + '.AccPmuModel.insert_pmu_origin_data'):
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

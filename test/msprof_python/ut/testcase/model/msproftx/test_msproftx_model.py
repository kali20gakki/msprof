import unittest
from unittest import mock

from msmodel.msproftx.msproftx_model import MsprofTxModel, MsprofTxExModel

NAMESPACE = 'msmodel.msproftx.msproftx_model'


class TestMsprofTxModel(unittest.TestCase):

    def test_flush(self):
        with mock.patch(NAMESPACE + '.MsprofTxModel.insert_data_to_db'):
            check = MsprofTxModel('test', 'msproftx.db', ['MsprofTx'])
            check.flush([])

    def test_get_timeline_data(self):
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data'):
            check = MsprofTxModel('test', 'msproftx.db', ['MsprofTx'])
            check.get_timeline_data()


class TestMsprofTxExModel(unittest.TestCase):

    def test_flush(self):
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data'):
            check = MsprofTxExModel('test')
            check.flush([])

    def test_get_timeline_data_should_return_true_when_get_data(self):
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[1]):
            check = MsprofTxExModel('test1')
            res = check.get_timeline_data()
        self.assertEqual(res, [1])

    def test_get_timeline_data_should_return_false_when_get_no_data(self):
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]):
            check = MsprofTxExModel('test2')
            res = check.get_timeline_data()
        self.assertEqual(res, [])

    def test_get_summary_data_should_return_true_when_get_data(self):
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[1]):
            check = MsprofTxExModel('test3')
            res = check.get_summary_data()
        self.assertEqual(res, [1])

    def test_get_summary_data_should_return_false_when_get_no_data(self):
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]):
            check = MsprofTxExModel('test4')
            res = check.get_summary_data()
        self.assertEqual(res, [])

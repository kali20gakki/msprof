import unittest
from unittest import mock

from msmodel.msproftx.msproftx_model import MsprofTxModel, MsprofTxExModel
from profiling_bean.db_dto.step_trace_dto import MsproftxMarkDto

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

    def test_get_summary_data_should_return_empty_list_when_no_table(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            check = MsprofTxModel('test', 'msproftx.db', ['MsprofTx'])
            ret = check.get_summary_data()
            self.assertEqual([], ret)


class TestMsprofTxExModel(unittest.TestCase):

    def test_flush(self):
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data'):
            check = MsprofTxExModel('test', 'msproftx.db', ['MsprofTxEx'])
            check.flush([])

    def test_get_timeline_data_should_return_true_when_get_data(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[1]):
                check = MsprofTxExModel('test1', 'msproftx.db', ['MsprofTxEx'])
                res = check.get_timeline_data()
        self.assertEqual(res, [1])

    def test_get_timeline_data_should_return_false_when_get_no_data(self):
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]):
            check = MsprofTxExModel('test2', 'msproftx.db', ['MsprofTxEx'])
            res = check.get_timeline_data()
        self.assertEqual(res, [])

    def test_get_summary_data_should_return_true_when_get_data(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[1]):
                check = MsprofTxExModel('test3', 'msproftx.db', ['MsprofTxEx'])
                res = check.get_summary_data()
        self.assertEqual(res, [1])

    def test_get_summary_data_should_return_false_when_get_no_data(self):
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]):
            check = MsprofTxExModel('test4', 'msproftx.db', ['MsprofTxEx'])
            res = check.get_summary_data()
        self.assertEqual(res, [])

    def test_get_device_summary_data_should_return_false_when_get_no_data(self):
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]):
            check = MsprofTxExModel('test5', 'step_trace.db', ['StepTrace'])
            res = check.get_device_data()
        self.assertEqual(res, [])

    def test_get_device_summary_data_should_return_true_when_last_data_is_range(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[
                    MsproftxMarkDto(0, 10, 0, 0),
                    MsproftxMarkDto(1, 11, 0, 1),
                    MsproftxMarkDto(1, 12, 0, 2)]):
            check = MsprofTxExModel('test6', 'step_trace.db', ['StepTrace'])
            res = check.get_device_data()
        self.assertEqual(res, [[0, 10, 0, 0, 0], [1, 11, 0, 1, 1]])

    def test_get_device_summary_data_should_return_true_when_last_data_is_mark(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[
                    MsproftxMarkDto(0, 10, 0, 0),
                    MsproftxMarkDto(0, 11, 0, 1),
                    MsproftxMarkDto(1, 12, 0, 2)]):
            check = MsprofTxExModel('test7', 'step_trace.db', ['StepTrace'])
            res = check.get_device_data()
        self.assertEqual(res, [[0, 10, 0, 0, 1], [1, 12, 0, 2, 0]])
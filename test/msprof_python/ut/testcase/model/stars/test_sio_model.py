import unittest
from unittest import mock

from msmodel.stars.sio_model import SioModel

NAMESPACE = 'msmodel.stars.sio_model'


class TestInterSocModel(unittest.TestCase):

    def test_flush(self):
        with mock.patch(NAMESPACE + '.SioModel.insert_data_to_db'):
            check = SioModel('test', 'test', [])
            check.flush([])

    def test_get_timeline_data_should_return_true_when_table_exist(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
             mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[1]):
            check = SioModel('test1', 'test2', [])
            res = check.get_timeline_data()
        self.assertEqual(res, [1])

    def test_get_timeline_data_should_return_false_when_table_not_exist(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            check = SioModel('test3', 'test4', [])
            res = check.get_timeline_data()
        self.assertEqual(res, [])

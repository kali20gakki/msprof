import unittest
from unittest import mock
from msmodel.stars.lowpower_model import LowPowerModel

NAMESPACE = 'msmodel.stars.lowpower_model'


class TestInterSocModel(unittest.TestCase):

    def test_flush(self):
        with mock.patch(NAMESPACE + '.LowPowerModel.insert_data_to_db'):
            check = LowPowerModel('test', 'test', [])
            check.flush([])

    def test_get_timeline_data(self):
        with mock.patch('msmodel.interface.base_model.DBManager.judge_table_exist', return_value=True), \
             mock.patch('msmodel.interface.base_model.DBManager.fetch_all_data', return_value=[1]):
            check = LowPowerModel('test', 'test', [])
            res = check.get_timeline_data()
        self.assertEqual(res, [1])

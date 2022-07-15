import unittest
from unittest import mock
from msmodel.stars.acsq_task_model import AcsqTaskModel

NAMESPACE = 'msmodel.stars.acsq_task_model'


class TestAcsqTaskModel(unittest.TestCase):

    def test_flush(self):
        with mock.patch(NAMESPACE + '.AcsqTaskModel.insert_data_to_db'):
            check = AcsqTaskModel('test', 'test', 'test')
            check.flush('test')

    def test_get_summary_data(self):
        with mock.patch(NAMESPACE + '.AcsqTaskModel.get_all_data', return_value=1):
            check = AcsqTaskModel('test', 'test', 'test')
            ret = check.get_summary_data()
        self.assertEqual(ret, 1)

    def test_get_timeline_data(self):
        with mock.patch(NAMESPACE + '.AcsqTaskModel.get_all_data', return_value=1):
            check = AcsqTaskModel('test', 'test', 'test')
            ret = check.get_timeline_data()
        self.assertEqual(ret, 1)

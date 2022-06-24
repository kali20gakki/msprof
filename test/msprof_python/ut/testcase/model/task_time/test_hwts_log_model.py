import unittest
from unittest import mock

from msmodel.task_time.hwts_log_model import HwtsLogModel

NAMESPACE = 'msmodel.task_time.hwts_log_model'


class TestHwtsLogModel(unittest.TestCase):

    def test_flush(self):
        with mock.patch(NAMESPACE + '.HwtsLogModel.insert_data_to_db'):
            check = HwtsLogModel('test')
            check.flush([], 'hwts')

    def test_clear(self):
        with mock.patch(NAMESPACE + '.PathManager.get_db_path'), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch('os.remove'):
            check = HwtsLogModel('test')
            check.clear()

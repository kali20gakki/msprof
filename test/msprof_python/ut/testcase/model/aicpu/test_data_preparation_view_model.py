import os
import unittest
from unittest import mock

from msmodel.ai_cpu.data_preparation_view_model import DataPreparationViewModel

NAMESPACE = 'msmodel.ai_cpu.data_preparation_view_model'


class TestDataPreparationViewModel(unittest.TestCase):
    DIR_PATH = os.path.join(os.path.dirname(__file__), 'DT_DataPreparationViewModel')

    def test_get_host_queue(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True),\
                mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]):
            DataPreparationViewModel(self.DIR_PATH).get_host_queue()

    def test_get_host_queue_mode(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True),\
                mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]):
            DataPreparationViewModel(self.DIR_PATH).get_host_queue_mode()

    def test_get_data_queue(self):
        with mock.patch(NAMESPACE + '.DataPreparationViewModel.get_all_data', return_value=True):
            DataPreparationViewModel(self.DIR_PATH).get_data_queue()


if __name__ == '__main__':
    unittest.main()
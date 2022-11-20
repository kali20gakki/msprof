import unittest
from unittest import mock

from msmodel.ai_cpu.data_preparation_model import DataPreparationModel

sample_config = {"model_id": 1, 'iter_id': 'dasfsd', 'result_dir': 'jasdfjfjs'}
NAMESPACE = 'msmodel.ai_cpu.data_preparation_model'


class TestDataPreparationModel(unittest.TestCase):

    def test_create_table(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False), \
                mock.patch(NAMESPACE + '.DBManager.sql_create_general_table'), \
                mock.patch(NAMESPACE + '.DBManager.execute_sql'):
            DataPreparationModel('test', ['test']).create_table()

    def test_flush(self):
        with mock.patch(NAMESPACE + '.DataPreparationModel.insert_data_to_db'):
            self.assertEqual(DataPreparationModel('test', ['test']).flush([]), None)

    def test_flush_all(self):
        with mock.patch(NAMESPACE + '.DataPreparationModel.insert_data_to_db'):
            DataPreparationModel('test', ['test']).flush_all({'test': []})


if __name__ == '__main__':
    unittest.main()

import sqlite3
import unittest
from unittest import mock

from msmodel.ai_cpu.data_queue_model import DataQueueModel

sample_config = {"model_id": 1, 'iter_id': 'dasfsd', 'result_dir': 'jasdfjfjs'}
NAMESPACE = 'msmodel.ai_cpu.data_queue_model'


class TestDataQueueModel(unittest.TestCase):

    def test_create_table(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False), \
             mock.patch(NAMESPACE + '.DBManager.sql_create_general_table'), \
             mock.patch(NAMESPACE + '.DBManager.execute_sql'):
            DataQueueModel('test', ['test']).create_table()

    def test_flush(self):
        with mock.patch(NAMESPACE + '.DataQueueModel.insert_data_to_db'):
            self.assertEqual(DataQueueModel('test', ['test']).flush([]), None)


if __name__ == '__main__':
    unittest.main()

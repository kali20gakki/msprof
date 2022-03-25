import sqlite3
import unittest
from unittest import mock

from model.ai_cpu.ai_cpu_model import AiCpuModel
from sqlite.db_manager import ConnDemo
from sqlite.db_manager import CursorDemo

sample_config = {"model_id": 1, 'iter_id': 'dasfsd', 'result_dir': 'jasdfjfjs'}
NAMESPACE = 'model.ai_cpu.ai_cpu_model'


class TestAiCpuModel(unittest.TestCase):

    def test_create_table(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False), \
             mock.patch(NAMESPACE + '.DBManager.sql_create_general_table'), \
             mock.patch(NAMESPACE + '.DBManager.execute_sql'):
            AiCpuModel('test', ['test']).create_table()

    def test_flush(self):
        with mock.patch(NAMESPACE + '.AiCpuModel.insert_data_to_db'):
            self.assertEqual(AiCpuModel('test', ['test']).flush([]), None)


if __name__ == '__main__':
    unittest.main()

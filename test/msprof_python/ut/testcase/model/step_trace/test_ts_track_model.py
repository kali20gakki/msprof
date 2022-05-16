import sqlite3
import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from model.step_trace.ts_track_model import TsTrackModel
from sqlite.db_manager import DBManager

NAMESPACE = 'model.step_trace.ts_track_model'


class TestTsTrackModel(unittest.TestCase):

    def test_flush(self):
        with mock.patch(NAMESPACE + '.TsTrackModel.insert_data_to_db'):
            check = TsTrackModel('test', 'step_trace.db', ['StepTrace', 'TsMemcpy'])
            check.flush('StepTrace', [])

    def test_create_table(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.sql_create_general_table'), \
                mock.patch(NAMESPACE + '.DBManager.drop_table'), \
                mock.patch(NAMESPACE + '.DBManager.execute_sql'):
            check = TsTrackModel('test', 'step_trace.db', ['StepTrace', 'TsMemcpy'])
            check.create_table('StepTrace')
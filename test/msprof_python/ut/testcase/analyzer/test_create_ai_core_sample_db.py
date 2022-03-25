import re
import struct
import unittest
from collections import OrderedDict
from unittest import mock
import sqlite3

import pytest

from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msprof_exception import ProfException
from constant.constant import CONFIG, INFO_JSON
from sqlite.db_manager import DBManager
from analyzer.create_ai_core_sample_db import ParsingCoreSampleData, \
    ParsingAICoreSampleData, ParsingAIVectorCoreSampleData

NAMESPACE = 'analyzer.create_ai_core_sample_db'


class TestParsingCoreSampleData(unittest.TestCase):

    def test_ms_run(self):
        ParsingCoreSampleData(CONFIG).ms_run()

    def test_sql_insert_metric_summary_table(self):
        metrics = ['total_time(ms)', "mac_fp16_ratio", "mac_int8_ratio"]
        freq = 100
        metric_key = 'ai_core_metrics'
        sql = 'SELECT cast(SUM((task_cyc*1000/(100))) as decimal(8,2)),' \
              'cast(1.0*SUM(r49)/SUM(task_cyc) as decimal(8,2)),' \
              'cast(1.0*SUM(r4a)/SUM(task_cyc) as decimal(8,2)) ' \
              'FROM EventCount where coreid = ?'
        with mock.patch(NAMESPACE + '.CalculateAiCoreData.add_fops_header'):
            check = ParsingCoreSampleData(CONFIG)
            result = check.sql_insert_metric_summary_table(metrics, freq, metric_key)
        self.assertEqual(result, sql)

    def test_read_binary_data(self):
        binary_data_path = 'test.slice_0'
        data = struct.pack("=BBHHH8QQQBBHHH",
                           1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 22, 33, 44, 55, 66, 77, 88, 99, 100)
        with mock.patch(NAMESPACE + '.PathManager.get_data_file_path', return_value=" "), \
             mock.patch(NAMESPACE + '.ParsingCoreSampleData._ParsingCoreSampleData__read_binary_data_helper'), \
             mock.patch('builtins.open', mock.mock_open(read_data=data)), \
             mock.patch('common_func.file_manager.check_path_valid'), \
             mock.patch(NAMESPACE + '.ParsingCoreSampleData.insert_binary_data', side_effect=OSError), \
             mock.patch(NAMESPACE + '.logging.error'):
            check = ParsingCoreSampleData(CONFIG)
            check.read_binary_data(binary_data_path)

    def test_insert_binary_data(self):
        with mock.patch(NAMESPACE + '.logging.error'), \
             mock.patch(NAMESPACE + '.ParsingCoreSampleData.insert_data', side_effect=sqlite3.Error):
            check = ParsingCoreSampleData(CONFIG)
            check.insert_binary_data("")

    def test_insert_metric_value(self):
        metrics_config = OrderedDict([('total_cycles', 'task_cyc'),
                                      ('scalar_ld_ratio', '1.0*SUM(r3a)/SUM(task_cyc)')])
        with mock.patch(NAMESPACE + '.read_cpu_cfg', return_value=False):
            check = ParsingCoreSampleData(CONFIG)
            result = check.insert_metric_value()
        self.assertEqual(result, 1)
        with mock.patch(NAMESPACE + '.read_cpu_cfg', return_value=metrics_config), \
             mock.patch(NAMESPACE + '.DBManager.execute_sql'), \
             mock.patch(NAMESPACE + '.DBManager.insert_data_into_table'), \
             mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[[0, 1]]):
            check = ParsingCoreSampleData(CONFIG)
            result = check.insert_metric_value()
        self.assertEqual(result, 0)

    def test_insert_metric_summary_table(self):
        with mock.patch(NAMESPACE + '.ParsingCoreSampleData._ParsingCoreSampleData__check_has_metrics',
                        return_value="1,2"), \
             mock.patch(NAMESPACE + '.ParsingCoreSampleData._ParsingCoreSampleData__check_is_valid_metrics'), \
             mock.patch(
                 NAMESPACE + '.ParsingCoreSampleData._ParsingCoreSampleData__insert_metric_summary_table_one_by_one'), \
             mock.patch(NAMESPACE + '.logging.error'), \
             mock.patch(NAMESPACE + '.logging.info'), \
             mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[[0, 1]]), \
             mock.patch(NAMESPACE + '.ParsingCoreSampleData.sql_insert_metric_summary_table', return_value=""), \
             mock.patch(NAMESPACE + '.error'):
            check = ParsingCoreSampleData(CONFIG)
            check.insert_metric_summary_table(0, "")

        with mock.patch(NAMESPACE + '.ParsingCoreSampleData._ParsingCoreSampleData__check_has_metrics',
                        return_value=""):
            check = ParsingCoreSampleData(CONFIG)
            check.insert_metric_summary_table(0, "")

        with mock.patch(NAMESPACE + '.ParsingCoreSampleData._ParsingCoreSampleData__check_has_metrics',
                        return_value="1,2"), \
             mock.patch(NAMESPACE + '.ParsingCoreSampleData._ParsingCoreSampleData__check_is_valid_metrics',
                        side_effect=OSError), \
             mock.patch(NAMESPACE + '.logging.error'), \
             mock.patch(NAMESPACE + '.logging.info'), \
             mock.patch(NAMESPACE + '.error'):
            check = ParsingCoreSampleData(CONFIG)
            check.insert_metric_summary_table(0, "")

    def test_insert_data(self):
        with mock.patch(NAMESPACE + '.logging.info'), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False), \
             mock.patch(NAMESPACE + '.ParsingCoreSampleData.create_ai_core_original_datatable'), \
             mock.patch(NAMESPACE + '.DBManager.executemany_sql'), \
             mock.patch(NAMESPACE + '.logging.error'), \
             mock.patch(NAMESPACE + '.error'):
            check = ParsingCoreSampleData(CONFIG)
            check.ai_core_data = [[1, 1, 1]]
            check.insert_data(" ")

    def test_create_ai_core_original_datatable(self):
        with mock.patch(NAMESPACE + '.DBManager.sql_create_general_table', return_value=""), \
             mock.patch(NAMESPACE + '.logging.error'), \
             mock.patch(NAMESPACE + '.error'), \
             pytest.raises(ProfException) as error:
            check = ParsingCoreSampleData(CONFIG)
            check.create_ai_core_original_datatable(" ")
            self.assertEqual(error.value.code, 10)

        with mock.patch(NAMESPACE + '.DBManager.sql_create_general_table', return_value="1"), \
             mock.patch(NAMESPACE + '.DBManager.execute_sql'):
            check = ParsingCoreSampleData(CONFIG)
            check.create_ai_core_original_datatable(" ")
            self.assertEqual(error.value.code, 10)

    def test_get_ai_core_event_chunk(self):
        event = [1, 1, 1, 1, 1, 1, 1, 1]
        with mock.patch(NAMESPACE + '.check_aicore_events'):
            check = ParsingCoreSampleData(CONFIG)
            result = check.get_ai_core_event_chunk(event)
        self.assertEqual(result, [[1, 1, 1, 1, 1, 1, 1, 1]])

    def test_insert_into_event_count_table(self):
        event = [1, 1, 1, 1, 1, 1, 1, 1]
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[[1]]), \
             mock.patch(NAMESPACE + '.DBManager.execute_sql'):
            check = ParsingCoreSampleData(CONFIG)
            result = check._ParsingCoreSampleData__insert_into_event_count_table(event, {})

    def test_create_ai_event_tables(self):
        event = [1, 1, 1, 1, 1, 1, 1, 1]
        with mock.patch(NAMESPACE + '.ParsingCoreSampleData._ParsingCoreSampleData__create_ai_event_tables_helper',
                        return_value=0):
            check = ParsingCoreSampleData(CONFIG)
            result = check.create_ai_event_tables(event)
        self.assertEqual(result, 0)

        with mock.patch(NAMESPACE + '.logging.error'), \
             mock.patch(NAMESPACE + '.ParsingCoreSampleData._ParsingCoreSampleData__create_ai_event_tables_helper',
                        side_effect=OSError):
            check = ParsingCoreSampleData(CONFIG)
            result = check.create_ai_event_tables(event)
        self.assertEqual(result, NumberConstant.ERROR)


class TestParsingAICoreSampleData(unittest.TestCase):
    def test_start_parsing_data_file(self):
        listdir = ['test1.slice_0', 'test2.slice_2']
        re_object = re.compile('^test2\\.(\\d+)\\.slice_\\d+').match('test2.0.slice_0')
        with mock.patch(NAMESPACE + '.PathManager.get_data_dir', return_value='test\\test'), \
             mock.patch(NAMESPACE + '.logging.error'), \
             mock.patch('os.listdir', return_value=listdir), \
                 mock.patch(NAMESPACE + '.get_file_name_pattern_match',
                            return_value=re_object), \
                 mock.patch(NAMESPACE + '.is_valid_original_data', return_value=True), \
                 mock.patch(NAMESPACE + '.DBManager.create_connect_db', return_value=(True, True)), \
                 mock.patch(NAMESPACE + '.DBManager.execute_sql'), \
                 mock.patch(NAMESPACE + '.ParsingAICoreSampleData.read_binary_data'), \
                 mock.patch(NAMESPACE + '.FileManager.add_complete_file'), \
                 mock.patch(NAMESPACE + '.logging.info'):
                check = ParsingAICoreSampleData(CONFIG)
                check.start_parsing_data_file()

    def test_create_aicore_db(self):
        config = CONFIG
        config['ai_core_profiling_mode'] = 'sample-based'
        db_manager = DBManager()
        res = db_manager.create_table('test.db')
        with mock.patch(NAMESPACE + '.ParsingAICoreSampleData.start_parsing_data_file'), \
             mock.patch(NAMESPACE + '.ParsingAICoreSampleData._process_device_list_ai_core',
                        side_effect=sqlite3.Error), \
             mock.patch(NAMESPACE + '.logging.error'):
            check = ParsingAICoreSampleData(config)
            check.create_ai_core_db()
        with mock.patch(NAMESPACE + '.ParsingAICoreSampleData.start_parsing_data_file',
                        return_value=1):
            check = ParsingAICoreSampleData(config)
            result = check.create_ai_core_db()
        self.assertEqual(result, None)
        with mock.patch(NAMESPACE + '.ParsingAICoreSampleData.start_parsing_data_file',
                        return_value=0), \
             mock.patch('sqlite3.connect', return_value=res[0]):
            with mock.patch(NAMESPACE + '.ParsingAICoreSampleData.create_ai_event_tables',
                            return_value=1):
                check = ParsingAICoreSampleData(config)
                check.device_list = [0]
                result = check.create_ai_core_db()
            self.assertEqual(result, None)
        db_manager = DBManager()
        res = db_manager.create_table('test.db')
        with mock.patch(NAMESPACE + '.ParsingAICoreSampleData.start_parsing_data_file',
                        return_value=0), \
             mock.patch('sqlite3.connect', return_value=res[0]):
            with mock.patch(NAMESPACE + '.ParsingAICoreSampleData.create_ai_event_tables',
                            return_value=0):
                with mock.patch(NAMESPACE + '.ParsingAICoreSampleData.insert_metric_value',
                                return_value=1):
                    check = ParsingAICoreSampleData(config)
                    check.device_list = [0]
                    result = check.create_ai_core_db()
                self.assertEqual(result, None)
        db_manager = DBManager()
        res = db_manager.create_table('test.db')
        with mock.patch(NAMESPACE + '.ParsingAICoreSampleData.start_parsing_data_file',
                        return_value=0), \
             mock.patch('sqlite3.connect', return_value=res[0]):
            with mock.patch(NAMESPACE + '.ParsingAICoreSampleData.create_ai_event_tables',
                            return_value=0):
                with mock.patch(NAMESPACE + '.ParsingAICoreSampleData.insert_metric_value',
                                return_value=0), \
                     mock.patch(NAMESPACE + '.ParsingAICoreSampleData.insert_metric_summary_table'):
                    check = ParsingAICoreSampleData(config)
                    check.device_list = [0]
                    InfoConfReader()._info_json = INFO_JSON
                    check.create_ai_core_db()

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.ParsingAICoreSampleData.create_ai_core_db',
                        side_effect=OSError), \
             mock.patch(NAMESPACE + '.logging.error'):
            check = ParsingAICoreSampleData(CONFIG)
            check.ms_run()


class TestParsingAIVectorCoreSampleData(unittest.TestCase):

    def test_start_parsing_data_file(self):
        listdir = ['test1.slice_0', 'test2.slice_2']
        re_object = re.compile('^test1\\.(\\d+)\\.slice_\\d+').match('test1.0.slice_0')
        with mock.patch(NAMESPACE + '.logging.error'), \
             mock.patch(NAMESPACE + '.PathManager.get_data_dir', return_value=''), \
             mock.patch('os.listdir', return_value=listdir), \
             mock.patch(NAMESPACE + '.get_file_name_pattern_match', return_value=re_object), \
             mock.patch(NAMESPACE + '.DBManager.create_connect_db', return_value=(True, True)), \
             mock.patch(NAMESPACE + '.DBManager.execute_sql'), \
             mock.patch(NAMESPACE + '.ParsingAIVectorCoreSampleData.read_binary_data'), \
             mock.patch(NAMESPACE + '.logging.info'):
            check = ParsingAIVectorCoreSampleData(CONFIG)
            check.start_parsing_data_file()

    def test_create_ai_vector_core_db(self):
        config = CONFIG
        config['aiv_profiling_mode'] = 'sample-based'
        db_manager = DBManager()
        res = db_manager.create_table('test.db')
        with mock.patch(NAMESPACE + '.ParsingAIVectorCoreSampleData.start_parsing_data_file'), \
             mock.patch(NAMESPACE + '.ParsingAIVectorCoreSampleData._process_device_list_ai_vector',
                        side_effect=sqlite3.Error), \
             mock.patch(NAMESPACE + '.logging.error'):
            check = ParsingAIVectorCoreSampleData(config)
            check.create_ai_vector_core_db()
        with mock.patch(NAMESPACE + '.ParsingAIVectorCoreSampleData.start_parsing_data_file',
                        return_value=1):
            check = ParsingAIVectorCoreSampleData(config)
            result = check.create_ai_vector_core_db()
        self.assertEqual(result, None)
        with mock.patch(NAMESPACE + '.ParsingAIVectorCoreSampleData.start_parsing_data_file',
                        return_value=0), \
             mock.patch('sqlite3.connect', return_value=res[0]):
            with mock.patch(NAMESPACE + '.ParsingAIVectorCoreSampleData.create_ai_event_tables',
                            return_value=1):
                check = ParsingAIVectorCoreSampleData(config)
                check.device_list = [0]
                result = check.create_ai_vector_core_db()
            self.assertEqual(result, None)
        db_manager = DBManager()
        res = db_manager.create_table('test.db')
        with mock.patch(NAMESPACE + '.ParsingAIVectorCoreSampleData.start_parsing_data_file',
                        return_value=0), \
             mock.patch('sqlite3.connect', return_value=res[0]):
            with mock.patch(NAMESPACE + '.ParsingAIVectorCoreSampleData.create_ai_event_tables',
                            return_value=0):
                with mock.patch(NAMESPACE + '.ParsingAIVectorCoreSampleData.insert_metric_value',
                                return_value=1):
                    check = ParsingAIVectorCoreSampleData(config)
                    check.device_list = [0]
                    result = check.create_ai_vector_core_db()
                self.assertEqual(result, None)
        db_manager = DBManager()
        res = db_manager.create_table('test.db')
        with mock.patch(NAMESPACE + '.ParsingAIVectorCoreSampleData.start_parsing_data_file',
                        return_value=0), \
             mock.patch('sqlite3.connect', return_value=res[0]):
            with mock.patch(NAMESPACE + '.ParsingAIVectorCoreSampleData.create_ai_event_tables',
                            return_value=0):
                with mock.patch(NAMESPACE + '.ParsingAIVectorCoreSampleData.insert_metric_value',
                                return_value=0), \
                     mock.patch(NAMESPACE + '.ParsingAIVectorCoreSampleData.insert_metric_summary_table'):
                    check = ParsingAIVectorCoreSampleData(config)
                    check.device_list = [0]
                    InfoConfReader()._info_json = INFO_JSON
                    check.create_ai_vector_core_db()

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.ParsingAIVectorCoreSampleData.create_ai_vector_core_db',
                        side_effect=OSError), \
             mock.patch(NAMESPACE + '.logging.error'):
            check = ParsingAIVectorCoreSampleData(CONFIG)
            check.ms_run()


if __name__ == '__main__':
    unittest.main()

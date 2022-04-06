import unittest
from unittest import mock
import configparser
import pytest

from common_func.msprof_exception import ProfException
from constant.constant import CONFIG, INFO_JSON
from analyzer.training.time_parser import TimeParser

NAMESPACE = 'analyzer.training.time_parser'


class TestTimeParser(unittest.TestCase):

    def test_parse_time(self):
        file_list = ['host_start.log.0']
        message = {'data': [
            {'device_id': 0, 'dev_mon': 100941307149, 'dev_wall': 1616557556918975294,
             'dev_cntvct': 4764985523, 'host_mon': 117891403077, 'host_wall': 1616557556918975294}
        ]}
        with mock.patch('os.listdir', return_value=file_list), \
                mock.patch(NAMESPACE + '.is_valid_original_data',
                           return_value=True), \
                mock.patch(NAMESPACE + '.TimeParser.get_sync_data',
                           return_value=message), \
                mock.patch(NAMESPACE + '.TimeSaver.save_data_to_db',
                           return_value=True), \
                mock.patch(NAMESPACE + '.FileManager.add_complete_file',
                           return_value=True):
            check = TimeParser(CONFIG)
            check.parse_time()

    def test_get_sync_data(self):
        message = {'data': [
            {'device_id': 0, 'dev_mon': 100941307149, 'dev_wall': 1616557556918975294,
             'dev_cntvct': 4764985523, 'host_mon': 117891403077, 'host_wall': 1616557556918975294}
        ]}
        with mock.patch('os.path.join', return_value='test\\data'), \
                mock.patch('os.path.exists', side_effect=OSError), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = TimeParser(CONFIG)
            result = check.get_sync_data('host_start.log.0')
        self.assertEqual(result, {})
        with mock.patch('os.path.join', return_value='test\\data'), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch(NAMESPACE + '.logging.info'), \
                mock.patch(NAMESPACE + '.TimeParser._TimeParser__parse_time_data',
                           return_value=message):
            check = TimeParser(CONFIG)
            result = check.get_sync_data('host_start.log.0')
        self.assertEqual(result, message)
        with mock.patch('os.path.join', return_value='test\\data'), \
                mock.patch('os.path.exists', return_value=False), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = TimeParser(CONFIG)
            res = check.get_sync_data('host_start.log.0')
        self.assertEqual(res, {})

    def test_parse_time_data(self):
        data = "clock_realtime: 1616557556918975294\n" + \
               "clock_monotonic_raw: 100941307149\n" + \
               "cntvct: 4764985523\n"
        wrong_data = "clock_realtime: a\n" + \
                     "clock_monotonic_raw: a\n" + \
                     "cntvct: a\n"
        wrong_data_1 = "clock_realtime: 123\n" + \
                       "clock_monotonic_raw: 145\n" + \
                       "cntvct: 2\n" + \
                       "a:b"
        with mock.patch('os.path.join', side_effect=OSError), \
                mock.patch(NAMESPACE + '.PathManager.get_sql_dir',
                           return_value='test\\data'), \
                mock.patch(NAMESPACE + '.check_path_valid'), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = TimeParser(CONFIG)
            result = check._TimeParser__parse_time_data()
        self.assertEqual(result, {"data": []})
        with mock.patch('os.path.join', return_value='test\\data'), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch('configparser.ConfigParser.read'), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch(NAMESPACE + '.check_path_valid'), \
                mock.patch(NAMESPACE + '.TimeParser.parse_host_start',
                           return_value=(1616557556918975294, 117891403077)):
            with mock.patch('builtins.open', mock.mock_open(read_data=data)), \
                    mock.patch(NAMESPACE + '.TimeParser.parse_dev_start',
                               return_value=(1616557556918975294, 100941307149, 4764985523)):
                check = TimeParser(CONFIG)
                result = check._TimeParser__parse_time_data()
            self.assertEqual(result['data'][0], {'dev_cntvct': 4764985523,
                                                 'dev_mon': 100941307149,
                                                 'dev_wall': 1616557556918975294,
                                                 'device_id': 0,
                                                 'host_mon': 117891403077,
                                                 'host_wall': 1616557556918975294})
            with mock.patch('builtins.open', mock.mock_open(read_data=wrong_data)):
                check = TimeParser(CONFIG)
                result = check._TimeParser__parse_time_data()
            self.assertEqual(result, {"data": []})
            with mock.patch('builtins.open', mock.mock_open(read_data=wrong_data_1)):
                check = TimeParser(CONFIG)
                result = check._TimeParser__parse_time_data()
            self.assertEqual(result['data'][0], {'dev_cntvct': 2,
                                                 'dev_mon': 145,
                                                 'dev_wall': 123,
                                                 'device_id': 0,
                                                 'host_mon': 117891403077,
                                                 'host_wall': 1616557556918975294})

    def test_parse_host_start(self):
        times = [('clock_realtime', '1616557556918975294'), ('clock_monotonic_raw', '117891403077'),
                 ('cntvct', '7049897983722348')]
        config = configparser.ConfigParser()
        with mock.patch('configparser.ConfigParser.items', return_value=times), \
             mock.patch('configparser.ConfigParser.has_section', return_value=True):
            check = TimeParser(CONFIG)
            result = check.parse_host_start(config, 0)
        self.assertEqual(result, ('1616557556918975294', '117891403077'))

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.TimeParser.parse_time', side_effect=OSError), \
                mock.patch(NAMESPACE + '.logging.error'):
            TimeParser(CONFIG).ms_run()

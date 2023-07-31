import configparser
import unittest
from unittest import mock

from constant.constant import CONFIG
from msparser.ms_timer.ms_time_parser import MsTimeParser

NAMESPACE = "msparser.ms_timer.ms_time_parser"


class TestMsTimeParser(unittest.TestCase):
    file_list = {}

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.MsTimeParser.parse'), \
                mock.patch(NAMESPACE + '.MsTimeParser.save'):
            check = MsTimeParser(self.file_list, CONFIG)
            check.ms_run()

    def test_parse(self):
        file_names = ['host_start.log.0']
        with mock.patch('common_func.file_name_manager' + '.get_host_start_compiles'), \
                mock.patch('common_func.file_name_manager' + '.get_file_name_pattern_match'), \
                mock.patch('os.listdir', return_value=file_names), \
                mock.patch('common_func.msvp_common' + '.is_valid_original_data'), \
                mock.patch('common_func.file_manager' + '.FileManager.add_complete_file'), \
                mock.patch(NAMESPACE + '.MsTimeParser.get_sync_data'):
            check = MsTimeParser(self.file_list, CONFIG)
            check.parse()

    def test_save(self):
        with mock.patch(NAMESPACE + '.MsTimeParser._check_time_format'), \
                mock.patch(NAMESPACE + '.MsTimeParser._pre_time_data'), \
                mock.patch('msmodel.ms_timer.ms_time_model.MsTimeModel.init'), \
                mock.patch('msmodel.ms_timer.ms_time_model.MsTimeModel.flush'), \
                mock.patch('msmodel.ms_timer.ms_time_model.MsTimeModel.finalize'), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = MsTimeParser(self.file_list, CONFIG)
            check.save()

    def test_get_sync_data(self):
        message = {'data': [
            {'device_id': 0, 'dev_mon': 100941307149, 'dev_wall': 1616557556918975294,
             'dev_cntvct': 4764985523, 'host_mon': 117891403077, 'host_wall': 1616557556918975294}
        ]}
        with mock.patch('os.path.join', return_value='test\\data'), \
                mock.patch('os.path.exists', side_effect=OSError), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = MsTimeParser(self.file_list, CONFIG)
            result = check.get_sync_data('host_start.log.0')
        self.assertEqual(result, {})
        with mock.patch('os.path.join', return_value='test\\data'), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch(NAMESPACE + '.logging.info'), \
                mock.patch(NAMESPACE + '.MsTimeParser._MsTimeParser__parse_time_data',
                           return_value=message):
            check = MsTimeParser(self.file_list, CONFIG)
            result = check.get_sync_data('host_start.log.0')
        self.assertEqual(result, message)
        with mock.patch('os.path.join', return_value='test\\data'), \
                mock.patch('os.path.exists', return_value=False), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = MsTimeParser(self.file_list, CONFIG)
            res = check.get_sync_data('host_start.log.0')
        self.assertEqual(res, {})

    def test_parse_host_start(self):
        times = [('clock_realtime', '1616557556918975294'), ('clock_monotonic_raw', '117891403077'),
                 ('cntvct', '7049897983722348')]
        config = configparser.ConfigParser()
        with mock.patch('configparser.ConfigParser.items', return_value=times), \
                mock.patch('configparser.ConfigParser.has_section', return_value=True):
            check = MsTimeParser(self.file_list, CONFIG)
            result = check.parse_host_start(config, 0)
        self.assertEqual(result, ('1616557556918975294', '117891403077'))

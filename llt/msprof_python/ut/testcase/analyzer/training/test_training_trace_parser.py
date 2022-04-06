import re
import struct
import unittest
from unittest import mock
from constant.constant import CONFIG, INFO_JSON
from analyzer.training.training_trace_parser import TrainingTraceParser
from common_func.info_conf_reader import InfoConfReader

NAMESPACE = 'analyzer.training.training_trace_parser'


class TestTimeParser(unittest.TestCase):
    config = CONFIG
    config['host_id'] = 1

    def test_parse_training_trace(self):
        file_list = ['training_trace.data.7.slice_0']
        with mock.patch('os.path.join', return_value='test\\data'), \
             mock.patch('os.listdir', return_value=file_list), \
             mock.patch(NAMESPACE + '.is_valid_original_data'):
            with mock.patch(NAMESPACE + '.TrainingTraceSaver.save_file_relation_to_db',
                            return_value=False):
                check = TrainingTraceParser(self.config)
                InfoConfReader()._info_json = INFO_JSON
                result = check.parse_training_trace()
            self.assertEqual(result, None)
            with mock.patch(NAMESPACE + '.TrainingTraceSaver.save_file_relation_to_db',
                            return_value=True), \
                 mock.patch(NAMESPACE + '.TrainingTraceParser.get_cp_data',
                            return_value={}), \
                 mock.patch(NAMESPACE + '.TrainingTraceSaver.save_data_to_db',
                            return_value=True):
                check = TrainingTraceParser(self.config)
                InfoConfReader()._info_json = INFO_JSON
                result = check.parse_training_trace()
            self.assertEqual(result, None)

    def test_parse_trace_data(self):
        check = TrainingTraceParser(self.config)
        with mock.patch(NAMESPACE + '.TrainingTraceParser._TrainingTraceParser__parse_trace_data_helper', side_effect=OSError):
            result = check._TrainingTraceParser__parse_trace_data('test', [1, 1, 2])
        self.assertEqual(result, ({}, 0))

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.TrainingTraceParser.parse_training_trace',
                        side_effect=OSError), \
             mock.patch(NAMESPACE + '.logging.error'):
            TrainingTraceParser(self.config).ms_run()

    def test_get_iter_by_training_trace(self):
        with mock.patch('os.path.basename', return_value='training_trace.data.7.slice_0'), \
             mock.patch('os.path.join', return_value="a"), \
             mock.patch('os.path.exists', return_value=True), \
             mock.patch(NAMESPACE + '.logging.error'), \
             mock.patch(NAMESPACE + '.logging.info'), \
             mock.patch('builtins.open', mock.mock_open(read_data="a")), \
             mock.patch(NAMESPACE + '.TrainingTraceParser._TrainingTraceParser__parse_trace_data',
                        return_value=(1,2)):
            check = TrainingTraceParser(self.config)
            result = check.get_cp_data('training_trace.data.7.slice_0')
        self.assertEqual(result, 1)


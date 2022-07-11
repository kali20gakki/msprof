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
        with mock.patch('common_func.file_name_manager' + '.get_host_start_compiles'), \
             mock.patch('common_func.file_name_manager' + '.get_file_name_pattern_match'), \
             mock.patch('os.listdir'), \
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

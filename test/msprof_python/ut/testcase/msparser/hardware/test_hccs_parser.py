import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from constant.constant import CONFIG
from msparser.hardware.hccs_parser import ParsingHCCSData
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.hardware.hccs_parser'


class TestParsingHCCSData(unittest.TestCase):
    file_list = {DataTag.HCCS: ['hccs.data.0.slice_0']}

    def test_read_binary_data(self):
        data = 'time_stamp(s):time_stamp(ns) send_amount receive_amount\n' + \
               '176168:391356200 409164109888 409164110240'
        with mock.patch('os.path.join'), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch('builtins.open', side_effect=OSError),\
             mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {"devices": '0'}
            check = ParsingHCCSData(self.file_list, CONFIG)
            check.read_binary_data('hccs.data.5.slice_0')
        with mock.patch('os.path.join'), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch('builtins.open', mock.mock_open(read_data=data)):
            InfoConfReader()._info_json = {"devices": '0'}
            check = ParsingHCCSData(self.file_list, CONFIG)
            check.read_binary_data('hccs.data.5.slice_0')

    def test_start_parsing_data_file(self):
        with mock.patch('os.path.join', side_effect=OSError), \
                mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {"devices": '0'}
            check = ParsingHCCSData(self.file_list, CONFIG)
            check.start_parsing_data_file()
        with mock.patch(NAMESPACE + '.is_valid_original_data', return_value=True), \
                mock.patch(NAMESPACE + '.logging.info'), \
                mock.patch(NAMESPACE + '.FileManager.add_complete_file'), \
                mock.patch(NAMESPACE + '.ParsingHCCSData.read_binary_data'):
            InfoConfReader()._info_json = {"devices": '0'}
            check = ParsingHCCSData(self.file_list, CONFIG)
            check.start_parsing_data_file()

    def test_save(self):
        with mock.patch('msmodel.hardware.hccs_model.HccsModel.init'), \
                mock.patch('msmodel.hardware.hccs_model.HccsModel.create_table'), \
                mock.patch('msmodel.hardware.hccs_model.HccsModel.flush'), \
                mock.patch('msmodel.hardware.hccs_model.HccsModel.insert_metrics'), \
                mock.patch('msmodel.hardware.hccs_model.HccsModel.finalize'):
            InfoConfReader()._info_json = {"devices": '0'}
            check = ParsingHCCSData(self.file_list, CONFIG)
            check.origin_data = [123]
            check.save()

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.ParsingHCCSData.start_parsing_data_file'), \
                mock.patch(NAMESPACE + '.ParsingHCCSData.save', side_effect=OSError), \
                mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {"devices": '0'}
            ParsingHCCSData(self.file_list, CONFIG).ms_run()

import struct
import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from constant.constant import CONFIG
from msparser.hardware.hbm_parser import ParsingHBMData
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.hardware.hbm_parser'


class TestParsingHBMData(unittest.TestCase):
    file_list = {DataTag.HBM: ['hbm.data.0.slice_0']}

    def test_read_binary_data(self):
        data = struct.pack("=QQIIQQIIQQIIQQII",
                           66, 0, 0, 0, 66, 0, 0, 1, 66, 0, 0, 2, 66, 0, 0, 3)
        with mock.patch('os.path.join', return_value='test\\test'), \
             mock.patch('os.path.exists', return_value=True), \
             mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch('struct.calcsize', return_value=False), \
                mock.patch('os.path.getsize', return_value=96):
            InfoConfReader()._info_json = {'devices': '0'}
            check = ParsingHBMData(self.file_list, CONFIG)
            result = check.read_binary_data('hbm.data.5.slice_0', "5", "0")
        self.assertEqual(result, 1)
        with mock.patch('os.path.join', return_value='test\\test'), \
             mock.patch('os.path.exists', return_value=True), \
             mock.patch('builtins.open', mock.mock_open(read_data=data)), \
             mock.patch(NAMESPACE + '.OffsetCalculator.pre_process', return_value=data), \
             mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch('os.path.getsize', return_value=96):
            InfoConfReader()._info_json = {'devices': '0'}
            check = ParsingHBMData(self.file_list, CONFIG)
            InfoConfReader()._start_log_time = 10000
            result = check.read_binary_data('hbm.data.5.slice_0', "5", "0")
        self.assertEqual(result, 0)

    def test_start_parsing_data_file(self):
        with mock.patch(NAMESPACE + '.is_valid_original_data', side_effect=OSError), \
             mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = ParsingHBMData(self.file_list, CONFIG)
            check.start_parsing_data_file()
        with mock.patch(NAMESPACE + '.is_valid_original_data', return_value=True), \
             mock.patch(NAMESPACE + '.logging.info'), \
             mock.patch(NAMESPACE + '.FileManager.add_complete_file'):
            with mock.patch(NAMESPACE + '.ParsingHBMData.read_binary_data',
                            return_value=0):
                InfoConfReader()._info_json = {'devices': '0'}
                check = ParsingHBMData(self.file_list, CONFIG)
                check.start_parsing_data_file()
            with mock.patch(NAMESPACE + '.ParsingHBMData.read_binary_data',
                            return_value=1), \
                 mock.patch(NAMESPACE + '.logging.error'):
                InfoConfReader()._info_json = {'devices': '0'}
                check = ParsingHBMData(self.file_list, CONFIG)
                check.start_parsing_data_file()

    def test_save(self):
        with mock.patch('msmodel.hardware.hbm_model.HbmModel.init'), \
             mock.patch('msmodel.hardware.hbm_model.HbmModel.create_table'), \
             mock.patch('msmodel.hardware.hbm_model.HbmModel.flush'), \
             mock.patch('msmodel.hardware.hbm_model.HbmModel.insert_bw_data'), \
             mock.patch('msmodel.hardware.hbm_model.HbmModel.finalize'):
            InfoConfReader()._info_json = {"devices": '0'}
            check = ParsingHBMData(self.file_list, CONFIG)
            check.hbm_data = [123]
            check.save()

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.ParsingHBMData.start_parsing_data_file'), \
             mock.patch(NAMESPACE + '.ParsingHBMData.save', side_effect=OSError), \
             mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {'devices': '0'}
            ParsingHBMData(self.file_list, CONFIG).ms_run()

import struct
import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from constant.constant import CONFIG
from msparser.hardware.tscpu_parser import ParsingTSData
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.hardware.tscpu_parser'


class TestParsingTSData(unittest.TestCase):
    file_list = {DataTag.TSCPU: ['tscpu.data.0.slice_0']}

    def test_is_mdc_binary_data(self):
        data = struct.pack("=L", 2880154539)
        wrong_data = struct.pack("=L", 2880154538)
        InfoConfReader()._info_json = {"devices": '0'}
        check = ParsingTSData(self.file_list, CONFIG)
        with mock.patch('builtins.open', mock.mock_open(read_data=wrong_data)):
            _wrong = open("test")
            result = check._is_mdc_binary_data(_wrong)
            self.assertEqual(result, False)
        with mock.patch('builtins.open', mock.mock_open(read_data=data)):
            true = open("test")
        result = check._is_mdc_binary_data(true)

    def test_read_binary_data(self):
        data = struct.pack("=LL20QLLQQ14QLL20QLLQQ14QLL20QLLQQ14Q",
                           2880154539, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 18446742993525863604, 176168475609540, 17, 38000,
                           8, 3570, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           2880154538, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 18446742993525863604, 176168475609540, 17, 38000,
                           8, 3570, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           2880154539, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 18446742993525863604, 176168475609540, 10000, 38000,
                           8, 3570, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           )
        with mock.patch(NAMESPACE + '.PathManager.get_data_file_path', return_value='test'), \
                mock.patch('os.path.getsize', return_value=struct.calcsize('=LL20QLLQQ14QLL20QLLQQ14QLL20QLLQQ14Q')), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch('os.path.join', return_value='test\\test'), \
                mock.patch('builtins.open', side_effect=SystemError), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = ParsingTSData(self.file_list, CONFIG)
            InfoConfReader()._info_json = {'devices': '0'}
            check.read_binary_data('test')
        with mock.patch(NAMESPACE + '.PathManager.get_data_file_path', return_value='test'), \
                mock.patch('os.path.getsize', return_value=struct.calcsize('=LL20QLLQQ14QLL20QLLQQ14QLL20QLLQQ14Q')), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch('builtins.open', mock.mock_open(read_data=data)), \
                mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = ParsingTSData(self.file_list, CONFIG)
            check.read_binary_data('test')

    def test_read_mdc_binary_data(self):
        data = struct.pack("=LL20LLLQ10QLL20LLLQ10QLL20LLLQ10Q",
                           2880154539, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, 263212560, 111542377976, 17, 768000, 8, 176049, 0,
                           0, 0, 0, 0, 0, 2880154538, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, 263212560, 111542377976, 17, 768000, 8, 176049, 0,
                           0, 0, 0, 0, 0, 2880154539, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, 263212560, 111542377976, 10000, 76800, 8, 176049, 0,
                           0, 0, 0, 0, 0, )
        with mock.patch(NAMESPACE + '.PathManager.get_data_file_path', return_value='test'), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch('os.path.getsize', return_value=struct.calcsize("=LL20LLLQ10QLL20LLLQ10QLL20LLLQ10Q")), \
                mock.patch('framework.offset_calculator.OffsetCalculator.pre_process', return_value=data), \
                mock.patch('builtins.open', side_effect=SystemError), \
                mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {"devices": '0'}
            check = ParsingTSData(self.file_list, CONFIG)
            check.read_mdc_binary_data('test')
        with mock.patch(NAMESPACE + '.PathManager.get_data_file_path', return_value='test'), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch("os.path.exists", return_valur=True), \
                mock.patch('os.path.getsize', return_value=struct.calcsize("=LL20LLLQ10QLL20LLLQ10QLL20LLLQ10Q")), \
                mock.patch('framework.offset_calculator.OffsetCalculator.pre_process', return_value=data), \
                mock.patch('builtins.open', mock.mock_open(read_data=data)), \
                mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {"devices": '0'}
            check = ParsingTSData(self.file_list, CONFIG)
            check.read_mdc_binary_data('test')
        self.assertEqual(len(check.ts_data), 5)

    def test_start_parsing_data_file(self):
        with mock.patch('os.path.join', return_value='test\\db'), \
                mock.patch(NAMESPACE + '.is_valid_original_data', side_effect=SystemError), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {"devices": '0'}
            check = ParsingTSData(self.file_list, CONFIG)
            result = check.start_parsing_data_file()
        self.assertEqual(result, None)
        with mock.patch('os.path.join', return_value='test\\db'), \
                mock.patch(NAMESPACE + '.is_valid_original_data', return_value=True), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch(NAMESPACE + '.logging.info'):
            with mock.patch(NAMESPACE + '.ParsingTSData._is_mdc_binary_data',
                            return_value=True), \
                    mock.patch(NAMESPACE + '.ParsingTSData.read_mdc_binary_data'):
                InfoConfReader()._info_json = {"devices": '0'}
                check = ParsingTSData(self.file_list, CONFIG)
                check.start_parsing_data_file()

        with mock.patch('os.path.join', return_value='test\\db'), \
                mock.patch(NAMESPACE + '.is_valid_original_data', return_value=True), \
                mock.patch(NAMESPACE + '.logging.info'), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch(NAMESPACE + '.ParsingTSData._is_mdc_binary_data',
                           return_value=False), \
                mock.patch(NAMESPACE + '.ParsingTSData.read_binary_data'):
            InfoConfReader()._info_json = {"devices": '0'}
            check = ParsingTSData(self.file_list, CONFIG)
            check.start_parsing_data_file()

    def test_save(self):
        with mock.patch('model.hardware.tscpu_model.TscpuModel.init'), \
                mock.patch('model.hardware.tscpu_model.TscpuModel.flush'), \
                mock.patch('model.hardware.tscpu_model.TscpuModel.finalize'):
            InfoConfReader()._info_json = {"devices": '0'}
            check = ParsingTSData(self.file_list, CONFIG)
            check.ts_data = [123]
            check.save()

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.ParsingTSData.start_parsing_data_file'), \
                mock.patch(NAMESPACE + '.ParsingTSData.start_parsing_data_file',
                           side_effect=RuntimeError), \
                mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {"devices": '0'}
            ParsingTSData(self.file_list, CONFIG).ms_run()

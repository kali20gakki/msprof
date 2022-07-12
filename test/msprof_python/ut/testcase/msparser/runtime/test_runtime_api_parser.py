import struct
import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from msparser.runtime.runtime_api_parser import RunTimeApiParser
from profiling_bean.prof_enum.data_tag import DataTag
from constant.constant import CONFIG

NAMESPACE = 'msparser.runtime.runtime_api_parser'


class TestRunTimeApiParser(unittest.TestCase):
    file_list = {DataTag.RUNTIME_API: ['runtime.api.1.slice_0']}

    def test_parse(self):
        with mock.patch(NAMESPACE + '.PathManager.get_data_file_path', return_value='test'), \
                mock.patch(NAMESPACE + '.RunTimeApiParser.read_binary_data'), \
                mock.patch('os.path.getsize', return_value=128), \
                mock.patch(NAMESPACE + '.FileManager.add_complete_file'):
            check = RunTimeApiParser(self.file_list, CONFIG)
            check.parse()
        with mock.patch(NAMESPACE + '.PathManager.get_data_file_path', return_value='test'), \
                mock.patch(NAMESPACE + '.RunTimeApiParser.read_binary_data'), \
                mock.patch('os.path.getsize', return_value=0), \
                mock.patch(NAMESPACE + '.FileManager.add_complete_file'):
            check = RunTimeApiParser(self.file_list, CONFIG)
            check.parse()
        with mock.patch(NAMESPACE + '.PathManager.get_data_file_path', side_effect=OSError), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = RunTimeApiParser(self.file_list, CONFIG)
            check.parse()

    def test_read_binary_data(self):
        data = struct.pack('=HHIQQQ64sIII10IH106B', 23130, 40, 0, 1, 2, 3, b'test', 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5, 6,
                           7, 8, 9, 1, 2, 3, 4, 5, 6, 4, 5, 6, 7, 8, 9, 4, 45, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 00, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
        with mock.patch(NAMESPACE + '.check_file_readable'), \
                mock.patch('builtins.open', mock.mock_open(read_data=data)), \
                mock.patch(NAMESPACE + '.OffsetCalculator.pre_process', return_value=data):
            check = RunTimeApiParser(self.file_list, CONFIG)
            check.read_binary_data('test', 256)
        with mock.patch(NAMESPACE + '.check_file_readable'), \
                mock.patch('builtins.open', side_effect=OSError), \
                mock.patch(NAMESPACE + '.OffsetCalculator.pre_process', return_value=data), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = RunTimeApiParser(self.file_list, CONFIG)
            check.read_binary_data('test', 256)

    def test_save(self):
        with mock.patch('msmodel.runtime.runtime_api_model.RuntimeApiModel.init'), \
                mock.patch('msmodel.runtime.runtime_api_model.RuntimeApiModel.create_table'), \
                mock.patch('msmodel.runtime.runtime_api_model.RuntimeApiModel.flush'), \
                mock.patch('msmodel.runtime.runtime_api_model.RuntimeApiModel.finalize'):
            InfoConfReader()._info_json = {"devices": '0'}
            check = RunTimeApiParser(self.file_list, CONFIG)
            check._data_list = [123]
            check.save()

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.RunTimeApiParser.parse'), \
                mock.patch(NAMESPACE + '.RunTimeApiParser.save'):
            RunTimeApiParser(self.file_list, CONFIG).ms_run()

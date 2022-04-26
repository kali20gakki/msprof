import unittest
from collections import OrderedDict
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from common_func.platform.chip_manager import ChipManager
from profiling_bean.prof_enum.chip_model import ChipModel

from constant.constant import CONFIG
from msparser.hardware.mini_llc_parser import MiniLLCParser
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.hardware.mini_llc_parser'


class TestMiniLLCParser(unittest.TestCase):
    file_list = {DataTag.LLC: ['llc.data.0.slice_0']}

    def test_handle_llc_data(self):
        tmp = OrderedDict([('device_id', 0), ('replayid', 0), ('timestamp', 1), ('read_allocate', 0),
                           ('read_noallocate', 0), ('read_hit', 0), ('write_allocate', 0),
                           ('write_noallocate', 0), ('write_hit', 0)])
        llc_data = [1466.706340169, '166393', "'hisi_l3c0_1", 'read_allocate']
        container = []
        device_id = 0
        replayid = 0
        InfoConfReader()._info_json = {"devices": '0'}
        check = MiniLLCParser(self.file_list, CONFIG)
        check.handle_llc_data(tmp, llc_data, container, device_id, replayid)
        self.assertEqual(tmp, OrderedDict([('device_id', 0),
                                           ('replayid', 0),
                                           ('timestamp', 1466.706340169),
                                           ('read_allocate', '166393'),
                                           ('read_noallocate', 0),
                                           ('read_hit', 0),
                                           ('write_allocate', 0),
                                           ('write_noallocate', 0),
                                           ('write_hit', 0)]))

    def test_read_binary_data(self):
        data = "# started on Sun Apr 25 18:22:32 2021\n" + \
               "#           time             counts unit events\n" + \
               "0.020715104             166393      'hisi_l3c0_1/read_allocate/abc\n" + \
               "test_continue            166393      'hisi_l3c0_1/read_allocate/\n" + \
               "0.020715104             166393      'hisi_l3c0_1/read_allocate/\n" + \
               "0.020715104             151150      hisi_l3c0_1/read_hit/\n" + \
               "0.020715104                 21      hisi_l3c0_1/read_noallocate/\n" + \
               "0.020715104              81601      hisi_l3c0_1/write_allocate/\n" + \
               "0.020715104              81601      hisi_l3c0_1/dsid_allocate/\n"
        with mock.patch('os.path.join', return_value='test\\data'):
            with mock.patch('builtins.open', side_effect=OSError), \
                    mock.patch(NAMESPACE + '.logging.error'), \
                    mock.patch(NAMESPACE + '.check_file_readable'):
                InfoConfReader()._info_json = {"devices": '0'}
                check = MiniLLCParser(self.file_list, CONFIG)
                InfoConfReader()._start_log_time = 1466685625065

                check.read_binary_data('llc.data.0.slice_0', 0, 0)
            with mock.patch('builtins.open', mock.mock_open(read_data=data)), \
                    mock.patch(NAMESPACE + '.check_file_readable'):
                InfoConfReader()._info_json = {"devices": '0'}
                check = MiniLLCParser(self.file_list, CONFIG)
                InfoConfReader()._start_log_time = 1466685625065
                check.read_binary_data('llc.data.0.slice_0', 0, 0)
            self.assertEqual(list(check.metric_tmp.values()),
                             [0, 0, 1466.706340169, '166393', '21', '151150', '81601', 0, 0])

    def test_start_parsing_data_file(self):
        with mock.patch(NAMESPACE + '.is_valid_original_data', side_effect=ValueError), \
                mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {"devices": '0'}
            check = MiniLLCParser(self.file_list, CONFIG)

            result = check.start_parsing_data_file()
        self.assertEqual(result, None)
        with mock.patch(NAMESPACE + '.is_valid_original_data', return_value=True), \
                mock.patch(NAMESPACE + '.logging.info'), \
                mock.patch(NAMESPACE + '.MiniLLCParser.read_binary_data'):
            InfoConfReader()._info_json = {"devices": '0'}
            check = MiniLLCParser(self.file_list, CONFIG)

            result = check.start_parsing_data_file()
        self.assertEqual(result, None)

    def test_save(self):
        with mock.patch('model.hardware.mini_llc_model.MiniLlcModel.init'), \
                mock.patch('model.hardware.mini_llc_model.MiniLlcModel.create_table'), \
                mock.patch('model.hardware.mini_llc_model.MiniLlcModel.flush'), \
                mock.patch('model.hardware.mini_llc_model.MiniLlcModel.calculate'), \
                mock.patch('model.hardware.mini_llc_model.MiniLlcModel.finalize'):
            InfoConfReader()._info_json = {"devices": '0'}
            check = MiniLLCParser(self.file_list, CONFIG)
            check.llc_data = [123]
            check.save()

    def test_run(self):
        with mock.patch(NAMESPACE + '.MiniLLCParser.start_parsing_data_file'), \
                mock.patch(NAMESPACE + '.MiniLLCParser.save',
                           side_effect=OSError), \
                mock.patch(NAMESPACE + '.logging.error'):
            ChipManager().chip_id = ChipModel.CHIP_V1_1_0
            InfoConfReader()._info_json = {"devices": '0'}
            MiniLLCParser(self.file_list, CONFIG).ms_run()

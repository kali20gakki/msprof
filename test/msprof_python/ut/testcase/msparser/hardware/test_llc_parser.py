import struct
import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from common_func.platform.chip_manager import ChipManager
from profiling_bean.prof_enum.chip_model import ChipModel

from constant.constant import CONFIG
from msparser.hardware.llc_parser import NonMiniLLCParser
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.hardware.llc_parser'


class TestNonMiniLLCParser(unittest.TestCase):
    file_list = {DataTag.LLC: ['llc.data.0.slice_0']}

    def test_read_binary_data(self):
        data = struct.pack("=QQIIQQII", 17152, 49307, 0, 0, 17152, 0, 1, 0)
        with mock.patch(NAMESPACE + '.PathManager.get_data_file_path', return_value='test'), \
             mock.patch('os.path.getsize', return_value=len(data)), \
             mock.patch('builtins.open', side_effect=OSError), \
             mock.patch('common_func.file_manager.check_path_valid'), \
             mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {"devices": '0'}
            check = NonMiniLLCParser(self.file_list, CONFIG)
            check.read_binary_data('llc.data.0.slice_0')
        with mock.patch(NAMESPACE + '.PathManager.get_data_file_path', return_value='test'), \
             mock.patch('os.path.getsize', return_value=len(data)), \
             mock.patch('common_func.file_manager.check_path_valid'), \
             mock.patch('builtins.open', mock.mock_open(read_data=data)):
            InfoConfReader()._info_json = {"devices": '0'}
            check = NonMiniLLCParser(self.file_list, CONFIG)
            check.read_binary_data('llc.data.0.slice_0')
        # self.assertEqual(check.origin_data,
        #                  [('0', 17152, 49307, 0, 0), ('0', 17152, 0, 1, 0)])

    def test_start_parsing_data_file(self):
        with mock.patch(NAMESPACE + '.is_valid_original_data', side_effect=ValueError), \
             mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {"devices": '0'}
            check = NonMiniLLCParser(self.file_list, CONFIG)

            result = check.start_parsing_data_file()
        self.assertEqual(result, None)
        with mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {"devices": '0'}
            check = NonMiniLLCParser(self.file_list, CONFIG)
            result = check.start_parsing_data_file()
        self.assertEqual(result, None)
        with mock.patch(NAMESPACE + '.is_valid_original_data', return_value=True), \
             mock.patch(NAMESPACE + '.logging.info'), \
             mock.patch(NAMESPACE + '.NonMiniLLCParser.read_binary_data'):
            InfoConfReader()._info_json = {"devices": '0'}
            check = NonMiniLLCParser(self.file_list, CONFIG)

            result = check.start_parsing_data_file()
        self.assertEqual(result, None)

    def test_save(self):
        with mock.patch('msmodel.hardware.llc_model.LlcModel.init'), \
             mock.patch('msmodel.hardware.llc_model.LlcModel.create_events_trigger'), \
             mock.patch('msmodel.hardware.llc_model.LlcModel.flush'), \
             mock.patch('msmodel.hardware.llc_model.LlcModel.create_table'), \
             mock.patch('msmodel.hardware.llc_model.LlcModel.insert_metrics_data'), \
             mock.patch('msmodel.hardware.llc_model.LlcModel.finalize'):
            InfoConfReader()._info_json = {"devices": '0'}
            check = NonMiniLLCParser(self.file_list, CONFIG)
            check.origin_data = [123]
            check.save()

    def test_run(self):
        with mock.patch(NAMESPACE + '.NonMiniLLCParser.start_parsing_data_file'), \
             mock.patch(NAMESPACE + '.NonMiniLLCParser.save',
                        side_effect=OSError), \
             mock.patch(NAMESPACE + '.logging.error'):
            ChipManager().chip_id = ChipModel.CHIP_V2_1_0
            InfoConfReader()._info_json = {"devices": '0'}
            NonMiniLLCParser(self.file_list, CONFIG).ms_run()

    def read_binary_data(self, file_name):
        """parsing llc data and insert into llc.db"""
        try:
            project_path = self.sample_config.get("result_dir", "")
            with open(os.path.join(project_path, 'data', file_name), 'rb') as llc_file:
                while True:
                    one_slice = llc_file.read(LLC_DATA_LEN)
                    if one_slice:
                        timestamp, count, event_id, l3t_id = struct.unpack(LLC_DATA, one_slice)
                        self.origin_data['llc_original_data'].append(
                            (self.device_id, timestamp, count, event_id, l3t_id))
                    else:
                        break
            if self.origin_data['llc_original_data']:
                self.insert_data()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error("%s: %s", file_name, err)

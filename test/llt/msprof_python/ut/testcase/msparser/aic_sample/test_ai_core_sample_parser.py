import struct
import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from constant.constant import CONFIG
from msparser.aic_sample.ai_core_sample_parser import ParsingCoreSampleData, \
    ParsingAICoreSampleData, ParsingAIVectorCoreSampleData
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.aic_sample.ai_core_sample_parser'


class TestParsingCoreSampleData(unittest.TestCase):

    def test_ms_run(self):
        InfoConfReader()._info_json = {'devices': '0'}
        ParsingCoreSampleData(CONFIG).ms_run()


class TestParsingAICoreSampleData(unittest.TestCase):
    file_list = {DataTag.AI_CORE: ['aicore.data.0.slice_0']}

    def test_read_binary_data(self):
        InfoConfReader()._info_json = {'devices': '0'}
        binary_data_path = 'test.slice_0'
        data = struct.pack("=BBHHH8QQQBBHHH",
                           1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 22, 33, 44, 55, 66, 77, 88, 99, 100)
        with mock.patch('os.path.getsize', return_value=len(data)), \
                mock.patch(NAMESPACE + '.PathManager.get_data_file_path'), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch('builtins.open', side_effect=OSError), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = ParsingAICoreSampleData(self.file_list, CONFIG)
            check.read_binary_data(binary_data_path)
        with mock.patch('os.path.getsize', return_value=len(data)), \
                mock.patch(NAMESPACE + '.PathManager.get_data_file_path'), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch('os.path.join', return_value='test\\test'), \
                mock.patch('builtins.open', mock.mock_open(
                    read_data=data)):
            check = ParsingAICoreSampleData(self.file_list, CONFIG)
            check.read_binary_data(binary_data_path)

    def test_start_parsing_data_file(self):
        InfoConfReader()._info_json = {'devices': '0'}
        with mock.patch('os.path.join', return_value='test\\test'), \
                mock.patch(NAMESPACE + '.logging.error'):
            with mock.patch(NAMESPACE + '.is_valid_original_data', side_effect=OSError):
                check = ParsingAICoreSampleData(self.file_list, CONFIG)
                check.start_parsing_data_file()
            with mock.patch(NAMESPACE + '.is_valid_original_data', return_value=True), \
                    mock.patch(NAMESPACE + '.ParsingAICoreSampleData.read_binary_data'), \
                    mock.patch(NAMESPACE + '.FileManager.add_complete_file'), \
                    mock.patch(NAMESPACE + '.logging.info'):
                check = ParsingAICoreSampleData(self.file_list, CONFIG)
                check.start_parsing_data_file()

    # def test_save(self): XXX
    #     InfoConfReader()._info_json = {'devices': '0',
    #                                    "DeviceInfo": [{'aic_frequency': '1150', 'aiv_frequency': '1000'}]}
    #     with mock.patch('model.aic.ai_core_sample_model.AiCoreSampleModel.init'), \
    #             mock.patch('model.aic.ai_core_sample_model.AiCoreSampleModel.insert_metric_summary_table'), \
    #             mock.patch('model.aic.ai_core_sample_model.AiCoreSampleModel.insert_metric_value'), \
    #             mock.patch('model.aic.ai_core_sample_model.AiCoreSampleModel.create_ai_vector_core_db'), \
    #             mock.patch('model.aic.ai_core_sample_model.AiCoreSampleModel.finalize'):
    #         check = ParsingAICoreSampleData(self.file_list, CONFIG)
    #         check.ai_core_data = [123]
    #         check.save()

    def test_ms_run(self):
        InfoConfReader()._info_json = {'devices': '0',
                                       "DeviceInfo": [{'aic_frequency': '1150', 'aiv_frequency': '1000'}]}
        with mock.patch(NAMESPACE + '.ParsingAICoreSampleData.start_parsing_data_file'), \
                mock.patch(NAMESPACE + '.ParsingAICoreSampleData.save', side_effect=OSError), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = ParsingAICoreSampleData(self.file_list, {'ai_core_profiling_mode': 'sample-based'})
            check.ms_run()
            check = ParsingAICoreSampleData(self.file_list, {'ai_core_profiling_mode': 'task-based'})
            check.ms_run()


class TestParsingAIVectorCoreSampleData(unittest.TestCase):
    file_list = {DataTag.AIV: ['aiVectorCore.data.0.slice_0']}

    def test_start_parsing_data_file(self):
        InfoConfReader()._info_json = {'devices': '0'}
        with mock.patch(NAMESPACE + '.is_valid_original_data', side_effect=OSError), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = ParsingAIVectorCoreSampleData(self.file_list, CONFIG)
            check.start_parsing_data_file()
        with mock.patch(NAMESPACE + '.is_valid_original_data', retuen_value=True), \
                mock.patch(NAMESPACE + '.ParsingAIVectorCoreSampleData.read_binary_data'), \
                mock.patch(NAMESPACE + '.logging.info'):
            check = ParsingAIVectorCoreSampleData(self.file_list, CONFIG)
            check.start_parsing_data_file()

    # def test_save(self): XXX
    #     InfoConfReader()._info_json = {'devices': '0',
    #                                    "DeviceInfo": [{'aic_frequency': '1150', 'aiv_frequency': '1000'}]}
    #     with mock.patch('model.aic.ai_core_sample_model.AiCoreSampleModel.init'), \
    #             mock.patch('model.aic.ai_core_sample_model.AiCoreSampleModel.insert_metric_summary_table'), \
    #             mock.patch('model.aic.ai_core_sample_model.AiCoreSampleModel.insert_metric_value'), \
    #             mock.patch('model.aic.ai_core_sample_model.AiCoreSampleModel.create_ai_vector_core_db'), \
    #             mock.patch('model.aic.ai_core_sample_model.AiCoreSampleModel.finalize'):
    #         check = ParsingAICoreSampleData(self.file_list, CONFIG)
    #         check.ai_core_data = [123]
    #         check.save()

    def test_ms_run(self):
        InfoConfReader()._info_json = {'devices': '0'}
        with mock.patch(NAMESPACE + '.ParsingAIVectorCoreSampleData.start_parsing_data_file'), \
                mock.patch(NAMESPACE + '.ParsingAIVectorCoreSampleData.save', side_effect=OSError), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = ParsingAIVectorCoreSampleData(self.file_list, {'ai_core_profiling_mode': 'sample-based'})
            check.ms_run()
            check = ParsingAIVectorCoreSampleData(self.file_list, {'ai_core_profiling_mode': 'task-based'})
            check.ms_run()


if __name__ == '__main__':
    unittest.main()

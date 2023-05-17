import struct
import unittest
from unittest import mock

from common_func.platform.chip_manager import ChipManager
from constant.constant import CONFIG
from constant.info_json_construct import DeviceInfo
from constant.info_json_construct import InfoJson
from constant.info_json_construct import InfoJsonReaderManager
from msparser.aic_sample.ai_core_sample_parser import ParsingAICoreSampleData
from msparser.aic_sample.ai_core_sample_parser import ParsingAIVectorCoreSampleData
from msparser.aic_sample.ai_core_sample_parser import ParsingCoreSampleData
from msparser.aic_sample.ai_core_sample_parser import ParsingFftsAICoreSampleData
from profiling_bean.prof_enum.chip_model import ChipModel
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.aic_sample.ai_core_sample_parser'


class TestParsingCoreSampleData(unittest.TestCase):

    def test_ms_run(self):
        InfoJsonReaderManager(info_json=InfoJson(devices='0')).process()
        ParsingCoreSampleData(CONFIG).ms_run()


class TestParsingFftsAICoreSampleData(unittest.TestCase):
    file_list = {DataTag.FFTS_PMU: ['ffts_profiler.data.0.slice_0']}

    def test_ms_run(self):
        InfoJsonReaderManager(info_json=InfoJson(devices='0')).process()
        ParsingFftsAICoreSampleData(self.file_list, CONFIG).ms_run()

    def test_read_binary_data(self):
        InfoJsonReaderManager(info_json=InfoJson(devices='0', platform_version='5')).process()
        binary_data_path = 'test.slice_0'
        data = struct.pack("=BBHHH8QQQBBHHHBBHHH8QQQBBHHH",
                           1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 22, 33, 44, 55, 66, 49, 88, 99, 100,
                           1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 22, 33, 44, 55, 66, 1, 88, 99, 100)

        with mock.patch('os.path.getsize', return_value=len(data)), \
                mock.patch(NAMESPACE + '.PathManager.get_data_file_path'), \
                mock.patch(NAMESPACE + '.OffsetCalculator.pre_process', return_value=data), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch('os.path.join', return_value='test\\test'), \
                mock.patch('builtins.open', mock.mock_open(
                    read_data=data)):
            check = ParsingFftsAICoreSampleData(self.file_list, CONFIG)
            ChipManager().chip_id = ChipModel.CHIP_V1_1_1
            check.read_binary_data(binary_data_path)

    def test_save(self):
        InfoJsonReaderManager(info_json=InfoJson(devices='0', DeviceInfo=[
            DeviceInfo(aic_frequency='1150', aiv_frequency='1000').device_info])).process()
        with mock.patch(NAMESPACE + '.check_aicore_events'), \
                mock.patch('msmodel.aic.ai_core_sample_model.AiCoreSampleModel.init'), \
                mock.patch('msmodel.aic.ai_core_sample_model.ConfigMgr.read_sample_config'), \
                mock.patch('msmodel.aic.ai_core_sample_model.AiCoreSampleModel.insert_metric_summary_table'), \
                mock.patch('msmodel.aic.ai_core_sample_model.AiCoreSampleModel.insert_metric_value'), \
                mock.patch('msmodel.aic.ai_core_sample_model.AiCoreSampleModel.create_core_table'), \
                mock.patch('msmodel.aic.ai_core_sample_model.AiCoreSampleModel.finalize'):
            check = ParsingFftsAICoreSampleData(self.file_list, CONFIG)
            check.data_dict = {
                'aic': {'data_list': [[66, 1, 0, 55.0, 1, 44, 6, 7, 8, 9, 10, 11, 22, 33]], 'db_name': 'aicore_0.db',
                        'event': ['0x50'], 'metrics_key': 'l2_cache'},
                'aiv': {'data_list': [[66, 1, 0, 55.0, 49, 44, 6, 7, 8, 9, 10, 11, 22, 33]],
                        'db_name': 'ai_vector_core_0.db', 'event': ['0x55'], 'metrics_key': 'l2_cache'}}
            check.save()


class TestParsingAICoreSampleData(unittest.TestCase):
    file_list = {DataTag.AI_CORE: ['aicore.data.0.slice_0']}

    def test_read_binary_data(self):
        InfoJsonReaderManager(info_json=InfoJson(devices='0')).process()
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
                mock.patch(NAMESPACE + '.OffsetCalculator.pre_process', return_value='test\\test'), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch('os.path.join', return_value='test\\test'), \
                mock.patch('builtins.open', mock.mock_open(
                    read_data=data)):
            check = ParsingAICoreSampleData(self.file_list, CONFIG)
            check.read_binary_data(binary_data_path)

    def test_start_parsing_data_file(self):
        InfoJsonReaderManager(info_json=InfoJson(devices='0')).process()
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

    def test_save(self):
        InfoJsonReaderManager(info_json=InfoJson(devices='0', DeviceInfo=[
            DeviceInfo(aic_frequency='1150', aiv_frequency='1000').device_info])).process()
        with mock.patch('msmodel.aic.ai_core_sample_model.AiCoreSampleModel.init'), \
                mock.patch('msmodel.aic.ai_core_sample_model.AiCoreSampleModel.insert_metric_summary_table'), \
                mock.patch('msmodel.aic.ai_core_sample_model.ConfigMgr.read_sample_config'), \
                mock.patch('msmodel.aic.ai_core_sample_model.AiCoreSampleModel.insert_metric_value'), \
                mock.patch('msmodel.aic.ai_core_sample_model.AiCoreSampleModel.create_core_table'), \
                mock.patch('msmodel.aic.ai_core_sample_model.AiCoreSampleModel.finalize'):
            check = ParsingAICoreSampleData(self.file_list, CONFIG)
            check.ai_core_data = [123]
            check.save()

    def test_ms_run(self):
        InfoJsonReaderManager(info_json=InfoJson(devices='0', DeviceInfo=[
            DeviceInfo(aic_frequency='1150', aiv_frequency='1000').device_info])).process()
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
        InfoJsonReaderManager(info_json=InfoJson(devices='0')).process()
        with mock.patch(NAMESPACE + '.is_valid_original_data', side_effect=OSError), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = ParsingAIVectorCoreSampleData(self.file_list, CONFIG)
            check.start_parsing_data_file()
        with mock.patch(NAMESPACE + '.is_valid_original_data', retuen_value=True), \
                mock.patch(NAMESPACE + '.ParsingAIVectorCoreSampleData.read_binary_data'), \
                mock.patch(NAMESPACE + '.logging.info'):
            check = ParsingAIVectorCoreSampleData(self.file_list, CONFIG)
            check.start_parsing_data_file()

    def test_save(self):
        InfoJsonReaderManager(info_json=InfoJson(devices='0', DeviceInfo=[
            DeviceInfo(aic_frequency='1150', aiv_frequency='1000').device_info])).process()
        with mock.patch('msmodel.aic.ai_core_sample_model.AiCoreSampleModel.init'), \
                mock.patch('msmodel.aic.ai_core_sample_model.AiCoreSampleModel.insert_metric_summary_table'), \
                mock.patch('msmodel.aic.ai_core_sample_model.ConfigMgr.read_sample_config'), \
                mock.patch('msmodel.aic.ai_core_sample_model.AiCoreSampleModel.create_core_table'), \
                mock.patch('msmodel.aic.ai_core_sample_model.AiCoreSampleModel.insert_metric_value'), \
                mock.patch('msmodel.aic.ai_core_sample_model.AiCoreSampleModel.finalize'):
            check = ParsingAICoreSampleData(self.file_list, CONFIG)
            check.ai_core_data = [123]
            check.save()

    def test_ms_run(self):
        InfoJsonReaderManager(info_json=InfoJson(devices='0')).process()
        with mock.patch(NAMESPACE + '.ParsingAIVectorCoreSampleData.start_parsing_data_file'), \
                mock.patch(NAMESPACE + '.ParsingAIVectorCoreSampleData.save', side_effect=OSError), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = ParsingAIVectorCoreSampleData(self.file_list, {'ai_core_profiling_mode': 'sample-based'})
            check.ms_run()
            check = ParsingAIVectorCoreSampleData(self.file_list, {'ai_core_profiling_mode': 'task-based'})
            check.ms_run()


if __name__ == '__main__':
    unittest.main()

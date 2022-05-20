import struct
import unittest
from unittest import mock

from constant.constant import CONFIG
from msparser.aicpu.parse_aicpu_data_adapter import ParseAiCpuDataAdapter
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.aicpu.parse_aicpu_data_adapter'


class TestParseAiCpuData(unittest.TestCase):
    file_list = {DataTag.AI_CPU: ['DATAPREPROCESS.aicpu.data.0.slice_0']}

    def test_get_ai_cpu_analysis_engine(self):
        with mock.patch(NAMESPACE + '.PathManager.get_data_file_path',
                        return_value='test\\data'), \
                mock.patch(NAMESPACE + '.logging.error'), \
                mock.patch('os.path.getsize', return_value=128), \
                mock.patch('common_func.msprof_iteration.MsprofIteration.get_iteration_end_dict', return_value={1: 10}):
            check = ParseAiCpuDataAdapter(self.file_list, CONFIG)
            result = check.get_ai_cpu_analysis_engine(self.file_list.get(DataTag.AI_CPU))
        self.assertTrue(result.__class__.__name__, 'ParseAiCpuData')

    def test_ms_run(self):
        data_bin = struct.pack("=HHHHI15Q", 0, 0, 23130, 60, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
        data = struct.pack("=HH16Q", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2)
        with mock.patch(NAMESPACE + '.PathManager.get_data_dir',
                        return_value='test\\data'), \
                mock.patch(NAMESPACE + '.PathManager.get_data_file_path',
                           return_value='test\\data\\DATA_PREPROCESS.AICPU.7.slice_0'), \
                mock.patch('os.path.getsize', return_value=132), \
                mock.patch('common_func.msprof_iteration.MsprofIteration.get_iteration_end_dict', return_value={1: 10}):
            with mock.patch('builtins.open', side_effect=SystemError), \
                    mock.patch(NAMESPACE + '.logging.error'), \
                    mock.patch('common_func.msprof_iteration.MsprofIteration.get_iteration_end_dict',
                               return_value={1: 10}), \
                    mock.patch('msparser.aicpu.parse_aicpu_data.ParseAiCpuData.ms_run', side_effect=SystemError):
                check = ParseAiCpuDataAdapter(self.file_list, CONFIG)
                check.ms_run()
        check = ParseAiCpuDataAdapter({DataTag.AI_CPU: []}, CONFIG)
        check.ms_run()

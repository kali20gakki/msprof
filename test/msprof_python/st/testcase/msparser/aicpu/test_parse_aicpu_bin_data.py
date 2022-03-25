import struct
import unittest
from unittest import mock

from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.info_conf_reader import InfoConfReader
from common_func.constant import Constant
from constant.constant import CONFIG
from msparser.aicpu.parse_aicpu_bin_data import ParseAiCpuBinData
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.aicpu.parse_aicpu_bin_data'


class TestParseAiCpuBinData(unittest.TestCase):
    file_list = {DataTag.AI_CPU: ['.data.0.slice_0']}

    def test_read_binary_data(self):
        data = struct.pack("=HHHHQQQQQQQIIQQQQIIIHBBQHHHHQQQQQQQIIQQQQIIIHBBQ",
                           23130, 60, 0, 0, 0, 1, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, 23130, 60, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
        with mock.patch('os.path.getsize', return_value=256), \
                mock.patch(NAMESPACE + '.OffsetCalculator.pre_process', return_value=data):
            with mock.patch('builtins.open', mock.mock_open(read_data=data)), \
                    mock.patch(NAMESPACE + '.logging.error'), \
                    mock.patch('common_func.msprof_iteration.Utils.is_step_scene', return_value=True), \
                    mock.patch('common_func.msprof_iteration.MsprofIteration.get_iteration_end_dict', return_value={1:10}):
                check = ParseAiCpuBinData(self.file_list, CONFIG)
                InfoConfReader()._info_json = {"DeviceInfo": [{'hwts_frequency': 100}]}
                check.read_binary_data('DATA_PREPROCESS.AICPU.7.slice_0')
            self.assertEqual(check.ai_cpu_datas,
                             [[0, '0', 1e-05, 2e-05, '', 0.0, 0.0, 1e-05, 0.0, 0.0, 0]])

    def test_parse_ai_cpu(self):
        with mock.patch(NAMESPACE + '.AiStackDataCheckManager.contain_dp_aicpu_data',
                        return_value=True), \
                mock.patch(NAMESPACE + '.is_valid_original_data',
                           return_value=True), \
                mock.patch(NAMESPACE + '.ParseAiCpuBinData.read_binary_data',
                           return_value=True), \
                mock.patch(NAMESPACE + '.PathManager.get_data_dir',
                           return_value='test'), \
                mock.patch(NAMESPACE + '.logging.info'), \
                mock.patch(NAMESPACE + '.FileManager.add_complete_file'), \
                mock.patch('common_func.msprof_iteration.Utils.is_step_scene', return_value=True):
            check = ParseAiCpuBinData(self.file_list, CONFIG)
            check.parse_ai_cpu()
        with mock.patch(NAMESPACE + '.AiStackDataCheckManager.contain_dp_aicpu_data',
                        return_value=False), \
             mock.patch('common_func.msprof_iteration.Utils.is_step_scene', return_value=True):
            check = ParseAiCpuBinData(self.file_list, CONFIG)
            result = check.parse_ai_cpu()
        self.assertEqual(result, None)

    def test_save(self):
        with mock.patch('model.ai_cpu.ai_cpu_model.AiCpuModel.init'), \
                mock.patch('model.ai_cpu.ai_cpu_model.AiCpuModel.create_table'), \
                mock.patch('model.ai_cpu.ai_cpu_model.AiCpuModel.flush'), \
                mock.patch('model.ai_cpu.ai_cpu_model.AiCpuModel.finalize'), \
                mock.patch('common_func.msprof_iteration.Utils.is_step_scene', return_value=True):
            InfoConfReader()._info_json = {"devices": '0'}
            check = ParseAiCpuBinData(self.file_list, CONFIG)
            check.ai_cpu_datas = [123]
            check.save()

    def test_ms_run(self):
        ProfilingScene().init("")
        ProfilingScene()._scene = Constant.STEP_INFO
        with mock.patch(NAMESPACE + '.ParseAiCpuBinData.parse_ai_cpu', side_effect=ValueError), \
                mock.patch(NAMESPACE + '.logging.error'), \
                mock.patch('common_func.msprof_iteration.Utils.is_step_scene', return_value=True):
            check = ParseAiCpuBinData(self.file_list, CONFIG)
            check.ms_run()

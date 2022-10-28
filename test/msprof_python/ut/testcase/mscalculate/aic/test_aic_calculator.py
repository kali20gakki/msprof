#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import struct
import unittest
from unittest import mock

from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.info_conf_reader import InfoConfReader
from constant.constant import CONFIG
from mscalculate.aic.aic_calculator import AicCalculator
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.struct_info.aic_pmu import AicPmuBean

NAMESPACE = 'mscalculate.aic.aic_calculator'


class TestAicCalculator(unittest.TestCase):
    file_list = {DataTag.AI_CORE: ['aicore.data.0.slice_0']}

    def test_get_total_aic_count(self):
        with mock.patch(NAMESPACE + '.PathManager.get_data_file_path', return_value='test'), \
                mock.patch("os.path.getsize", return_value=1000):
            check = AicCalculator(self.file_list, CONFIG)
            result = check._get_total_aic_count()
            self.assertEqual(result, 7)

    def test_get_offset_and_total(self):
        with mock.patch(NAMESPACE + '.AicCalculator._get_total_aic_count', return_value=7), \
                mock.patch('msmodel.iter_rec.iter_rec_model.HwtsIterModel.get_task_offset_and_sum',
                           return_value=(0, 0)), \
                mock.patch('msmodel.iter_rec.iter_rec_model.HwtsIterModel.get_aic_sum_count',
                           return_value=8):
            check = AicCalculator(self.file_list, CONFIG)
            result = check._get_offset_and_total(0, 1)
            self.assertEqual(result, (0, -1))

    def test_parse_by_iter(self):
        with mock.patch('msmodel.iter_rec.iter_rec_model.HwtsIterModel.check_db', return_value=True), \
                mock.patch('msmodel.iter_rec.iter_rec_model.HwtsIterModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + '.MsprofIteration.get_iteration_id_by_index_id', return_value=10):
            with mock.patch(NAMESPACE + '.AicCalculator._get_offset_and_total', return_value=(127, 1280)), \
                    mock.patch(NAMESPACE + '.AicCalculator._parse'), \
                    mock.patch('framework.offset_calculator.FileCalculator.prepare_process'), \
                    mock.patch('msmodel.aic.aic_pmu_model.AicPmuModel.finalize'):
                check = AicCalculator(self.file_list, CONFIG)
                check._parse_by_iter()
            with mock.patch(NAMESPACE + '.AicCalculator._get_offset_and_total', return_value=(128, 0)), \
                    mock.patch(NAMESPACE + '.logging.warning'):
                check = AicCalculator(self.file_list, CONFIG)
                check._parse_by_iter()

    def test_parse_all_file(self):
        aic_reader = struct.pack("=BBHHHII10Q8I", 1, 2, 3, 4, 5, 6, 78, 9, 1, 0, 1, 2, 3, 4, 6, 5, 2, 4, 5, 6, 7, 89, 9,
                                 7, 4)
        with mock.patch(NAMESPACE + '.PathManager.get_data_file_path', return_value='test'), \
                mock.patch(NAMESPACE + '.logging.info'), \
                mock.patch(NAMESPACE + '.AicCalculator._parse'), \
                mock.patch('builtins.open', mock.mock_open(read_data=aic_reader)), \
                mock.patch('os.path.getsize', return_value=len(aic_reader)):
            check = AicCalculator(self.file_list, CONFIG)
            check._parse_all_file()

    def test_calculate(self):
        with mock.patch(NAMESPACE + '.AicCalculator._parse_all_file'), \
                mock.patch('common_func.utils.Utils.get_scene', return_value='single_op'), \
                mock.patch(NAMESPACE + '.AicCalculator._parse_by_iter'):
            check = AicCalculator(self.file_list, CONFIG)
            check.calculate()
        ProfilingScene().init('test')
        with mock.patch(NAMESPACE + '.AicCalculator._parse_all_file'), \
                mock.patch('common_func.utils.Utils.get_scene', return_value='train'), \
                mock.patch(NAMESPACE + '.AicCalculator._parse_by_iter'):
            check = AicCalculator(self.file_list, CONFIG)
            check.calculate()

    def test_parse(self):
        aic_reader = struct.pack("=BBHHHII10Q8I", 1, 2, 3, 4, 5, 6, 78, 9, 1, 0, 1, 2, 3, 4, 6, 5, 2, 4, 5, 6, 7, 89, 9,
                                 7, 4)
        with mock.patch(NAMESPACE + '.AicPmuUtils.get_pmu_events', return_value=['']), \
                mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config',
                           return_value={'ai_core_profiling_events': 'task-based'}), \
                mock.patch(NAMESPACE + '.AicCalculator.calculate_pmu_list'):
            check = AicCalculator(self.file_list, CONFIG)
            check._parse(aic_reader)

    def test_calculate_pmu_list(self):
        aic_reader = struct.pack("=BBHHHII10Q8I", 1, 2, 3, 4, 5, 6, 78, 9, 1, 0, 1, 2, 3, 4, 6, 5, 2, 4, 5, 6, 7, 89, 9,
                                 7, 4)
        _aic_pmu_log = AicPmuBean.decode(aic_reader)
        with mock.patch(NAMESPACE + '.CalculateAiCoreData.compute_ai_core_data', return_value=('', {})), \
                mock.patch(NAMESPACE + '.Utils.generator_to_list', return_value=[]):
            check = AicCalculator(self.file_list, CONFIG)
            check.calculate_pmu_list(_aic_pmu_log, [], [])

    def test_save(self):
        with mock.patch('msmodel.aic.aic_pmu_model.AicPmuModel.init'), \
                mock.patch('msmodel.aic.aic_pmu_model.AicPmuModel.flush'), \
                mock.patch('msmodel.aic.aic_pmu_model.AicPmuModel.finalize'):
            InfoConfReader()._info_json = {"devices": '0'}
            check = AicCalculator(self.file_list, CONFIG)
            check._aic_data_list = [123]
            check.save()

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.generate_config', return_value={'ai_core_profiling_mode': 'sample-based'}), \
                mock.patch(NAMESPACE + '.PathManager.get_sample_json_path', return_value='test'):
            check = AicCalculator(self.file_list, CONFIG)
            check.ms_run()
        with mock.patch(NAMESPACE + '.generate_config', return_value={'ai_core_profiling_mode': 'task-based'}), \
                mock.patch(NAMESPACE + '.PathManager.get_sample_json_path', return_value='test'), \
                mock.patch(NAMESPACE + '.AicCalculator.calculate', return_value='test'), \
                mock.patch(NAMESPACE + '.AicCalculator.save', return_value='test'):
            check = AicCalculator(self.file_list, CONFIG)
            check.ms_run()

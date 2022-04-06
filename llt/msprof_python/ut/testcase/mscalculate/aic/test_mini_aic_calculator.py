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
from mscalculate.aic.mini_aic_calculator import MiniAicCalculator
from profiling_bean.prof_enum.data_tag import DataTag


NAMESPACE = 'mscalculate.aic.mini_aic_calculator'


class TestMiniMiniAicCalculator(unittest.TestCase):
    file_list = {DataTag.AI_CORE: ['aicore.data.0.slice_0']}

    def test_parse_ai_core_pmu_event(self):
        with mock.patch(NAMESPACE + '.generate_config',
                        return_value={'ai_core_profiling_events': '0x64,0x65,0x66'}), \
                mock.patch(NAMESPACE + '.AicPmuUtils.get_pmu_event_name'),\
                mock.patch(NAMESPACE + '.PathManager.get_sample_json_path', return_value='test'):
            check = MiniAicCalculator(self.file_list, CONFIG)
            check._parse_ai_core_pmu_event()

    def test_insert_metric_summary(self):
        InfoConfReader()._info_json = {'DeviceInfo': [{'aic_frequency': 115}]}
        with mock.patch(NAMESPACE + '.MiniAicCalculator._parse_ai_core_pmu_event'), \
                mock.patch(NAMESPACE + '.insert_metric_summary_table'):
            check = MiniAicCalculator(self.file_list, CONFIG)
            check.insert_metric_summary()

    def test_op_insert_metric_summary(self):
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='test'), \
                mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True):
            check = MiniAicCalculator(self.file_list, CONFIG)
            check.op_insert_metric_summary()
        InfoConfReader()._info_json = {'DeviceInfo': [{'aic_frequency': 115}]}
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='test'), \
                mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=False), \
                mock.patch(NAMESPACE + '.MiniAicCalculator._parse_ai_core_pmu_event'), \
                mock.patch(NAMESPACE + '.insert_metric_summary_table'):
            check = MiniAicCalculator(self.file_list, CONFIG)
            check.op_insert_metric_summary()

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.generate_config', return_value={'ai_core_profiling_mode': 'sample-based'}), \
                mock.patch(NAMESPACE + '.PathManager.get_sample_json_path', return_value='test'):
            check = MiniAicCalculator(self.file_list, CONFIG)
            check.ms_run()
        ProfilingScene()._scene = "step_info"
        with mock.patch(NAMESPACE + '.generate_config', return_value={'ai_core_profiling_mode': 'task-based'}), \
                mock.patch(NAMESPACE + '.PathManager.get_sample_json_path', return_value='test'), \
                mock.patch(NAMESPACE + '.MiniAicCalculator.insert_metric_summary', return_value='test'):
            check = MiniAicCalculator(self.file_list, CONFIG)
            check.ms_run()
        ProfilingScene().init('')
        ProfilingScene()._scene = "single_op"
        with mock.patch(NAMESPACE + '.generate_config', return_value={'ai_core_profiling_mode': 'task-based'}), \
                mock.patch(NAMESPACE + '.PathManager.get_sample_json_path', return_value='test'), \
                mock.patch(NAMESPACE + '.MiniAicCalculator.op_insert_metric_summary', return_value='test'):
            check = MiniAicCalculator(self.file_list, CONFIG)
            check.ms_run()

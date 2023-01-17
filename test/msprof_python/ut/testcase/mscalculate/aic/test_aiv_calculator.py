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
from mscalculate.aic.aiv_calculator import AivCalculator
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'mscalculate.aic.aiv_calculator'


class TestAivCalculator(unittest.TestCase):
    file_list = {DataTag.AIV: ['aiVectorCore.data.0.slice_0']}

    def test_get_total_aic_count(self):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch("os.path.getsize", return_value=1000):
            check = AivCalculator(self.file_list, CONFIG)
            result = check._get_total_aic_count()
            self.assertEqual(result, 7)

    def test_calculate(self):
        with mock.patch(NAMESPACE + '.AivCalculator._parse_all_file'), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch('common_func.utils.Utils.get_scene', return_value='single_op'), \
                mock.patch(NAMESPACE + '.AivCalculator._parse_by_iter'):
            check = AivCalculator(self.file_list, CONFIG)
            check.calculate()
        ProfilingScene().init('test')
        with mock.patch(NAMESPACE + '.AivCalculator._parse_all_file'), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch('common_func.utils.Utils.get_scene', return_value='train'), \
                mock.patch(NAMESPACE + '.AivCalculator._parse_by_iter'):
            check = AivCalculator(self.file_list, CONFIG)
            check.calculate()

    def test_parse(self):
        aic_reader = struct.pack("=BBHHHII10Q8I", 1, 2, 3, 4, 5, 6, 78, 9, 1, 0, 1, 2, 3, 4, 6, 5, 2, 4, 5, 6, 7, 89, 9,
                                 7, 4)
        with mock.patch(NAMESPACE + '.AicPmuUtils.get_pmu_events', return_value=['']), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.AivCalculator.calculate_pmu_list'):
            check = AivCalculator(self.file_list, CONFIG)
            check._core_num_dict = {'aic': 30, 'aiv': 0}
            check._block_dims = {'2-2': [22, 22]}
            check._freq = 1500
            check._parse(aic_reader)

    def test_save(self):
        with mock.patch('msmodel.aic.aic_pmu_model.AicPmuModel.init'), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch('msmodel.aic.aic_pmu_model.AicPmuModel.flush'), \
                mock.patch('msmodel.aic.aic_pmu_model.AicPmuModel.finalize'):
            InfoConfReader()._info_json = {"devices": '0'}
            check = AivCalculator(self.file_list, CONFIG)
            check._aiv_data_list = [123]
            check.save()

    def test_ms_run(self):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config",
                        return_value={'aiv_profiling_mode': "sample-based"}):
            check = AivCalculator({}, CONFIG)
            check.ms_run()
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.AivCalculator.calculate', return_value='test'), \
                mock.patch(NAMESPACE + '.AivCalculator.init_params', return_value='test'), \
                mock.patch(NAMESPACE + '.AivCalculator._parse_all_file', return_value='test'), \
                mock.patch(NAMESPACE + '.AivCalculator.save', return_value='test'):
            check = AivCalculator(self.file_list, CONFIG)
            check.ms_run()

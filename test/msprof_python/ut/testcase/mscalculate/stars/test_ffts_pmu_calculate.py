#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2022. All rights reserved.
"""
from unittest import TestCase
from unittest import mock

from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.constant import Constant
from common_func.info_conf_reader import InfoConfReader
from constant.constant import CONFIG
from mscalculate.stars.ffts_pmu_calculate import FftsPmuCalculate
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.stars.ffts_pmu import FftsPmuBean

NAMESPACE = 'mscalculate.stars.ffts_pmu_calculate'


class TestFftsPmuCalculate(TestCase):
    file_list = {DataTag.FFTS_PMU: ['ffts_profile.data.0.slice_0']}

    def setUp(self):
        ProfilingScene().init('')

    def test_ms_run_sample_based(self):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.generate_config',
                           return_value={"ai_core_profiling_mode": "sample-based",
                                         "aiv_profiling_mode": "sample-based"}):
            check = FftsPmuCalculate(self.file_list, CONFIG)
            check.ms_run()

    def test_ms_run_task_based(self):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config",
                        return_value={"ai_core_profiling_mode": "task-based", "aiv_profiling_mode": "task-based"}), \
                mock.patch(NAMESPACE + '.FftsPmuCalculate.calculate'), \
                mock.patch(NAMESPACE + '.FftsPmuCalculate.init_params'), \
                mock.patch(NAMESPACE + '.FftsPmuCalculate.save'):
            check = FftsPmuCalculate(self.file_list, CONFIG)
            check.ms_run()

    def test_calculate_by_iter_no_table(self):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.Utils.get_scene', return_value=Constant.STEP_INFO), \
                mock.patch("os.path.exists", return_value=True), \
                mock.patch("os.remove"), \
                mock.patch("msmodel.interface.base_model.BaseModel.init"), \
                mock.patch("msmodel.interface.base_model.BaseModel.finalize"), \
                mock.patch("msmodel.interface.base_model.BaseModel.create_table"), \
                mock.patch(NAMESPACE + '.HwtsIterModel.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.HwtsIterModel.check_table', return_value=False):
            check = FftsPmuCalculate(self.file_list, CONFIG)
            check.calculate()

    def test_calculate_by_iter_table_exist_and_save(self):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.Utils.get_scene', return_value=Constant.STEP_INFO), \
                mock.patch("os.path.exists", return_value=True), \
                mock.patch("os.remove"), \
                mock.patch("msmodel.interface.base_model.BaseModel.init"), \
                mock.patch("msmodel.interface.base_model.BaseModel.finalize"), \
                mock.patch("msmodel.interface.base_model.BaseModel.create_table"), \
                mock.patch(NAMESPACE + '.HwtsIterModel.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.HwtsIterModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + '.HwtsIterModel.get_task_offset_and_sum', return_value=[0, 100]), \
                mock.patch(NAMESPACE + '.PathManager.get_data_file_path'), \
                mock.patch('os.path.getsize', return_value=1280), \
                mock.patch(NAMESPACE + '.HwtsIterModel.get_aic_sum_count', return_value=50), \
                mock.patch(NAMESPACE + '.FileCalculator.prepare_process',
                           return_value=b'\xe8\x01\xd3k\x0b\x00\x02\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00'
                                        b'\x00\x00\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x1ek1\x00\x00\x00\x00'
                                        b'\x00\xff\xff\xff\xff\xff\xff\xff\xff\x97+\x00\x00\x00\x00\x00\x00\x00\x00'
                                        b'\x00\x00\x00\x00\x00\x00\x16\x8a\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
                                        b'\x00\x00\x00\x00`\x00\x00\x00\x00\x00\x00\x00\x95Z.\x00\x00\x00\x00\x00'
                                        b'\x976\x00\x00\x00\x00\x00\x00\xb4\n\x00\x00\x00\x00\x00\x00\x05\x19+'
                                        b'\xafH\x03\x00\x00\xe9"+\xafH\x03\x00\x00'), \
                mock.patch(NAMESPACE + ".FftsPmuModel.create_table"), \
                mock.patch(NAMESPACE + '.FftsPmuModel.flush'):
            check = FftsPmuCalculate(self.file_list, CONFIG)
            InfoConfReader()._info_json = {'DeviceInfo': [{'aic_frequency': 1500, 'hwts_frequency': 1000}]}
            check._core_num_dict = {'aic': 30, 'aiv': 0}
            check._block_dims = {'2-2': [22, 22]}
            check._freq = 1500
            check.calculate()
            check.save()

    def test_calculate_all_file(self):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.Utils.get_scene', return_value=Constant.SINGLE_OP), \
                mock.patch(NAMESPACE + '.PathManager.get_data_file_path'), \
                mock.patch(NAMESPACE + '.check_file_readable'), \
                mock.patch(NAMESPACE + '.FileOpen'), \
                mock.patch('os.path.getsize', return_value=128), \
                mock.patch(NAMESPACE + '.OffsetCalculator.pre_process',
                           return_value=b'\xe8\x01\xd3k\x0b\x00\x02\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00'
                                        b'\x00\x00\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x1ek1\x00\x00\x00\x00'
                                        b'\x00\xff\xff\xff\xff\xff\xff\xff\xff\x97+\x00\x00\x00\x00\x00\x00\x00\x00'
                                        b'\x00\x00\x00\x00\x00\x00\x16\x8a\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
                                        b'\x00\x00\x00\x00`\x00\x00\x00\x00\x00\x00\x00\x95Z.\x00\x00\x00\x00\x00'
                                        b'\x976\x00\x00\x00\x00\x00\x00\xb4\n\x00\x00\x00\x00\x00\x00\x05\x19+\xafH'
                                        b'\x03\x00\x00\xe9"+\xafH\x03\x00\x00'):
            check = FftsPmuCalculate(self.file_list, CONFIG)
            check.calculate()

    def test_save_no_data(self):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.logging.warning'):
            check = FftsPmuCalculate(self.file_list, CONFIG)
            check.save()

    def test_calculate_block_pmu_list(self):
        data_1 = [1, 2, 3, 4, 5, 6, 7, 89, 9, 1, 4, 5, 6, 789, 9, 5, 5, 5, 6, 7, 9, 54, 1, 55, 5]
        data_2 = [1, 2, 3, 4, 5, 6, 7, 32768, 9, 1, 4, 5, 6, 789, 9, 5, 5, 5, 6, 7, 9, 9, 8, 7]
        pmu_data_list = [FftsPmuBean(data_1), FftsPmuBean(data_2)]
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config'), \
                mock.patch(NAMESPACE + '.AicPmuUtils.get_pmu_events',
                           return_value=['vec_ratio', 'mac_ratio', 'scalar_ratio',
                                         'mte1_ratio', 'mte2_ratio', 'mte3_ratio',
                                         'icache_req_ratio', 'icache_miss_rate']), \
                mock.patch(NAMESPACE + '.FftsPmuCalculate._get_pmu_value'):
            check = FftsPmuCalculate(self.file_list, CONFIG)
            check.calculate_block_pmu_list(pmu_data_list[1])
            check.calculate_block_pmu_list(pmu_data_list[0])

    def test_add_block_pmu_list(self):
        block_dict = {43 - 13 - 0: [['MIX_AIC', (349456, (1161, 0, 1088, 0, 7809, 7380, 391, 13))],
                                    ['MIX_AIC', (349425, (1161, 0, 1121, 0, 7915, 7409, 407, 13))]],
                      43 - 14 - 0: [['MIX_AIV', (349456, (1161, 0, 1088, 0, 7809, 7380, 391, 13))]],
                      43 - 14 - 0: [['MIX_OTHER', (349456, (1161, 0, 1088, 0, 7809, 7380, 391, 13))]]}
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config'), \
                mock.patch(NAMESPACE + '.AicPmuUtils.get_pmu_events',
                           return_value=['vec_ratio', 'mac_ratio', 'scalar_ratio',
                                         'mte1_ratio', 'mte2_ratio', 'mte3_ratio',
                                         'icache_req_ratio', 'icache_miss_rate']), \
                mock.patch(NAMESPACE + '.FftsPmuCalculate._get_pmu_value'):
            check = FftsPmuCalculate(self.file_list, CONFIG)
            check.block_dict = block_dict
            check.add_block_pmu_list()

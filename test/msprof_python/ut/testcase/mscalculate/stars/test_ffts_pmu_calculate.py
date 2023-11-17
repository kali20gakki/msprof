#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2022. All rights reserved.
"""
from unittest import TestCase
from unittest import mock

from common_func.constant import Constant
from common_func.info_conf_reader import InfoConfReader
from common_func.profiling_scene import ProfilingScene
from constant.constant import CONFIG
from mscalculate.stars.ffts_pmu_calculator import FftsPmuCalculator
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.stars.ffts_pmu import FftsPmuBean

NAMESPACE = 'mscalculate.stars.ffts_pmu_calculator'


class TestFftsPmuCalculator(TestCase):
    file_list = {DataTag.FFTS_PMU: ['ffts_profile.data.0.slice_0']}

    def setUp(self):
        ProfilingScene().init('')

    def test_ms_run_sample_based(self):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config",
                        return_value={"ai_core_profiling_mode": "sample-based",
                                      "aiv_profiling_mode": "sample-based"}):
            check = FftsPmuCalculator(self.file_list, CONFIG)
            check.ms_run()

    def test_ms_run_task_based(self):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config",
                        return_value={"ai_core_profiling_mode": "task-based", "aiv_profiling_mode": "task-based"}), \
                mock.patch(NAMESPACE + '.FftsPmuCalculator.calculate'), \
                mock.patch(NAMESPACE + '.FftsPmuCalculator.init_params'), \
                mock.patch(NAMESPACE + '.FftsPmuCalculator.save'):
            check = FftsPmuCalculator(self.file_list, CONFIG)
            check.ms_run()

    def test_calculate_by_iter_no_table(self):
        ProfilingScene().set_all_export(False)
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.Utils.get_scene', return_value=Constant.STEP_INFO), \
                mock.patch("os.path.exists", return_value=True), \
                mock.patch("os.remove"), \
                mock.patch("msmodel.interface.base_model.BaseModel.init"), \
                mock.patch("msmodel.interface.base_model.BaseModel.finalize"), \
                mock.patch("msmodel.interface.base_model.BaseModel.create_table"), \
                mock.patch(NAMESPACE + '.HwtsIterModel.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.HwtsIterModel.check_table', return_value=False):
            check = FftsPmuCalculator(self.file_list, CONFIG)
            check.calculate()

    def test_calculate_by_iter_table_exist_and_save(self):
        ProfilingScene().set_all_export(False)
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
            check = FftsPmuCalculator(self.file_list, CONFIG)
            InfoConfReader()._info_json = {'DeviceInfo': [{'aic_frequency': 1500, 'hwts_frequency': 1000}]}
            check._core_num_dict = {'aic': 30, 'aiv': 0}
            check._block_dims = {'2-2': [22, 22]}
            check._freq = 1500
            check.calculate()
            check.save()

    def test_save_should_be_success_when__is_mix_needed_is_true(self):
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
                mock.patch(NAMESPACE + ".FileOpen"), \
                mock.patch(NAMESPACE + ".FileOpen.file_reader.read"), \
                mock.patch('common_func.file_manager.check_path_valid'), \
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
            mixCalcute = FftsPmuCalculator(self.file_list, CONFIG)
            InfoConfReader()._info_json = {'DeviceInfo': [{'aic_frequency': 1500, 'hwts_frequency': 1000}]}
            mixCalcute._core_num_dict = {'aic': 30, 'aiv': 0}
            mixCalcute._block_dims = {'2-2': [22, 22]}
            mixCalcute._freq = 1500
            mixCalcute.calculate()
            mixCalcute._is_mix_needed = True
            mixCalcute.save()

    def test_calculate_all_file(self):
        ProfilingScene().set_all_export(True)
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
            check = FftsPmuCalculator(self.file_list, CONFIG)
            check.calculate()

    def test_calculate_when_table_exist_then_do_not_execute(self):
        ProfilingScene().set_all_export(True)
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.Utils.get_scene', return_value=Constant.SINGLE_OP), \
                mock.patch('common_func.db_manager.DBManager.check_tables_in_db', return_value=True), \
                mock.patch('logging.info'):
            check = FftsPmuCalculator(self.file_list, CONFIG)
            check._parse_all_file = mock.Mock()
            check.calculate()
            check._parse_all_file.assert_not_called()

    def test_save_no_data(self):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.logging.warning'):
            check = FftsPmuCalculator(self.file_list, CONFIG)
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
                mock.patch(NAMESPACE + '.FftsPmuCalculator._get_pmu_value'):
            check = FftsPmuCalculator(self.file_list, CONFIG)
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
                mock.patch(NAMESPACE + '.FftsPmuCalculator._get_pmu_value'):
            check = FftsPmuCalculator(self.file_list, CONFIG)
            check.block_dict = block_dict
            check.add_block_pmu_list()

    def test_get_current_freq(self):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}):
            check = FftsPmuCalculator(self.file_list, CONFIG)
            check._freq = 1850000000
            check.freq_data = []
            self.assertEqual(check._get_current_freq(1500), 1850000000)
            check.freq_data = [[1000, 0], [2000, 0], [4000, 0]]

            self.assertEqual(check._get_current_freq(1500), 1850000000)
            check.freq_data = [[1000, 1600], [2000, 1500], [4000, 800]]

            self.assertEqual(check._get_current_freq(1500), 1600000000)
            self.assertEqual(check._get_current_freq(2200), 1500000000)
            self.assertEqual(check._get_current_freq(4600), 800000000)

    def test_calculate_total_time_when_get_total_cycle_then_return_total_time(self):
        data = [1, 2, 3, 4, 5, 6, 7, 89, 9, 1, 4, 5, 6, 789, 9, 5, 5, 5, 6, 7, 9, 54, 1, 55, 5]
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.FftsPmuCalculator._is_not_mix_main_core', side_effect=[True, False]), \
                mock.patch(NAMESPACE + '.FftsPmuCalculator._get_current_freq', return_value=40), \
                mock.patch(NAMESPACE + '.Utils.cal_total_time', return_value=100):
            check = FftsPmuCalculator(self.file_list, CONFIG)
            check._core_num_dict = {"aic": 20}
            check._freq = 1800
            check.freq_data = [[1000, 1800], [2000, 800], [4000, 1800]]
            block_dim_dict = {False: 20, True: 40}
            self.assertEqual(check.calculate_total_time(FftsPmuBean(data), 200, block_dim_dict), 100)

    def test_set_ffts_table_name_list_should_set_table_aic_and_aiv_name_when_mix_needed_is_True(self):
        table_name_list = [
            "total_time(ms)", "total_cycles", "ub_read_bw(GB/s)", "ub_write_bw(GB/s)", "l1_read_bw(GB/s)",
            "l1_write_bw(GB/s)", "l2_read_bw(GB/s)", "l2_write_bw(GB/s)", "main_mem_read_bw(GB/s)",
            "main_mem_write_bw(GB/s)"
        ]
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.get_metrics_from_sample_config',
                           side_effect=[table_name_list, table_name_list]):
            check = FftsPmuCalculator(self.file_list, CONFIG)
            check._is_mix_needed = True
            check._set_ffts_table_name_list()
            self.assertEqual(check.aic_table_name_list, table_name_list[2:])
            self.assertEqual(check.aiv_table_name_list, table_name_list[2:])

    def test_set_ffts_table_name_list_should_set_table_aic_name_when_mix_needed_is_False(self):
        table_name_list = [
            "total_time(ms)", "total_cycles", "ub_read_bw(GB/s)", "ub_write_bw(GB/s)", "l1_read_bw(GB/s)",
            "l1_write_bw(GB/s)", "l2_read_bw(GB/s)", "l2_write_bw(GB/s)", "main_mem_read_bw(GB/s)",
            "main_mem_write_bw(GB/s)"
        ]
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.get_metrics_from_sample_config',
                           side_effect=[table_name_list, table_name_list]):
            check = FftsPmuCalculator(self.file_list, CONFIG)
            check._is_mix_needed = False
            check._set_ffts_table_name_list()
            self.assertEqual(check.aic_table_name_list, table_name_list[2:])
            self.assertEqual(check.aiv_table_name_list, [])

    def test_set_ffts_table_name_list_should_set_new_aic_name_when_metric_is_pipelineExecuteUtilization(self):
        table_name_list = [
            "total_time(ms)", "total_cycles", "vec_exe_time", "vec_exe_ratio",
            "mac_time", "mac_ratio_extra", "scalar_time", "scalar_ratio",
            "mte1_time", "mte1_ratio_extra", "mte2_time", "mte2_ratio",
            "mte3_time", "mte3_ratio", "fixpipe_time", "fixpipe_ratio"
        ]
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config",
                        return_value={"ai_core_metrics": Constant.PMU_PIPE_EXECUT}), \
                mock.patch(NAMESPACE + '.get_metrics_from_sample_config',
                           side_effect=[table_name_list, table_name_list]):
            check = FftsPmuCalculator(self.file_list, CONFIG)
            check._is_mix_needed = False
            check._set_ffts_table_name_list()
            self.assertEqual(check.aic_table_name_list, table_name_list[2:])
            self.assertEqual(check.aiv_table_name_list, [])

#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import struct
import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.profiling_scene import ProfilingScene
from constant.constant import CONFIG
from constant.constant import ITER_RANGE
from mscalculate.aic.aic_calculator import AicCalculator
from mscalculate.aic.aic_calculator import NanoAicCalculator
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.struct_info.aic_pmu import AicPmuBean

NAMESPACE = 'mscalculate.aic.aic_calculator'


class TestAicCalculator(unittest.TestCase):
    file_list = {DataTag.AI_CORE: ['aicore.data.0.slice_0']}

    def test_get_total_aic_count(self):
        with mock.patch(NAMESPACE + '.PathManager.get_data_file_path', return_value='test'), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch("os.path.getsize", return_value=1000):
            check = AicCalculator(self.file_list, CONFIG)
            result = check._get_total_aic_count()
            self.assertEqual(result, 7)

    def test_get_offset_and_total(self):
        with mock.patch(NAMESPACE + '.AicCalculator._get_total_aic_count', return_value=7), \
                mock.patch('msmodel.iter_rec.iter_rec_model.HwtsIterModel.get_task_offset_and_sum',
                           return_value=(0, 0)), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch('msmodel.iter_rec.iter_rec_model.HwtsIterModel.get_aic_sum_count',
                           return_value=8):
            check = AicCalculator(self.file_list, CONFIG)
            result = check._get_offset_and_total(ITER_RANGE)
            self.assertEqual(result, (0, -1))

    def test_parse_by_iter(self):
        with mock.patch('msmodel.iter_rec.iter_rec_model.HwtsIterModel.check_db', return_value=True), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch('msmodel.iter_rec.iter_rec_model.HwtsIterModel.check_table', return_value=True):
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
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch("common_func.file_manager.check_path_valid"), \
                mock.patch(NAMESPACE + '.logging.info'), \
                mock.patch(NAMESPACE + '.AicCalculator._parse'), \
                mock.patch('builtins.open', mock.mock_open(read_data=aic_reader)), \
                mock.patch('os.path.getsize', return_value=len(aic_reader)):
            check = AicCalculator(self.file_list, CONFIG)
            check._parse_all_file()

    def test_calculate(self):
        with mock.patch(NAMESPACE + '.AicCalculator._parse_all_file'), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch('common_func.utils.Utils.get_scene', return_value='single_op'):
            check = AicCalculator(self.file_list, CONFIG)
            check.calculate()
        ProfilingScene().init('test')
        with mock.patch(NAMESPACE + '.AicCalculator._parse_all_file'), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch('common_func.utils.Utils.get_scene', return_value='train'):
            check = AicCalculator(self.file_list, CONFIG)
            check.calculate()

    def test_calculate_when_table_exist_then_do_not_execute(self):
        ProfilingScene().set_all_export(True)
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch('common_func.db_manager.DBManager.check_tables_in_db', return_value=True), \
                mock.patch('logging.info'):
            check = AicCalculator(self.file_list, CONFIG)
            check._parse_all_file = mock.Mock()
            check.calculate()
            check._parse_all_file.assert_not_called()

    def test_calculate_when_not_all_export_then_parse_by_iter(self):
        ProfilingScene().set_all_export(False)
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.AicCalculator.pre_parse'):
            check = AicCalculator(self.file_list, CONFIG)
            check._parse_by_iter = mock.Mock(return_value=None)
            check.calculate()
            check._parse_by_iter.assert_called_once()
        ProfilingScene().set_all_export(True)

    def test_parse(self):
        aic_reader = struct.pack("=BBHHHII10Q8I", 1, 2, 3, 4, 5, 6, 78, 9, 1, 0, 1, 2, 3, 4, 6, 5, 2, 4, 5, 6, 7, 89, 9,
                                 7, 4)
        with mock.patch(NAMESPACE + '.AicPmuUtils.get_pmu_events', return_value=['']), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.AicCalculator.calculate_pmu_list'):
            check = AicCalculator(self.file_list, CONFIG)
            check._core_num_dict = {'aic': 30, 'aiv': 0}
            check._block_dims = {'2-2': [22, 22]}
            check._freq = 1500
            check._parse(aic_reader)

    def test_calculate_pmu_list(self):
        aic_reader = struct.pack("=BBHHHII10Q8I", 1, 2, 3, 4, 5, 6, 78, 9, 1, 0, 1, 2, 3, 4, 6, 5, 2, 4, 5, 6, 7, 89, 9,
                                 7, 4)
        _aic_pmu_log = AicPmuBean.decode(aic_reader)
        with mock.patch(NAMESPACE + '.CalculateAiCoreData.compute_ai_core_data', return_value=('', {})), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.Utils.generator_to_list', return_value=[]):
            check = AicCalculator(self.file_list, CONFIG)
            check.calculate_pmu_list(_aic_pmu_log, [], [], 0)

    def test_save(self):
        with mock.patch('msmodel.aic.aic_pmu_model.AicPmuModel.init'), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch('msmodel.aic.aic_pmu_model.AicPmuModel.flush'), \
                mock.patch('msmodel.aic.aic_pmu_model.AicPmuModel.finalize'):
            InfoConfReader()._info_json = {"devices": '0'}
            check = AicCalculator(self.file_list, CONFIG)
            check._aic_data_list = [123]
            check.save()

    def test_ms_run(self):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.PathManager.get_sample_json_path', return_value='test'):
            check = AicCalculator({}, CONFIG)
            check.ms_run()
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.PathManager.get_sample_json_path', return_value='test'), \
                mock.patch(NAMESPACE + '.AicCalculator.init_params', return_value='test'), \
                mock.patch(NAMESPACE + '.AicCalculator.calculate', return_value='test'), \
                mock.patch(NAMESPACE + '.AicCalculator.save', return_value='test'):
            check = AicCalculator(self.file_list, CONFIG)
            check.ms_run()


class TestNanoAicCalculator(unittest.TestCase):

    def test_calculate_when_nomal_then_pass(self):
        with mock.patch('msmodel.nano.nano_stars_model.NanoStarsViewModel.get_nano_pmu_details'), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch("mscalculate.aic.aic_utils.AicPmuUtils.get_pmu_events"), \
                mock.patch('mscalculate.aic.aic_calculator.AicCalculator._core_num_dict'), \
                mock.patch('common_func.utils.Utils.cal_total_time'), \
                mock.patch('mscalculate.aic.aic_calculator.AicCalculator.calculate_pmu_list'):
            check = NanoAicCalculator({}, CONFIG)
            check._aic_data_list = [123]
            check.calculate()

    def test_calculate_when_table_exist_then_do_not_execute(self):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch('common_func.db_manager.DBManager.check_tables_in_db', return_value=True), \
                mock.patch('logging.info'):
            check = NanoAicCalculator({}, CONFIG)
            check._aic_data_list = [123]
            check._parse_without_decode = mock.Mock()
            check.calculate()
            check._parse_without_decode.assert_not_called()

    def test_save_when_nomal_then_pass(self):
        with mock.patch('msmodel.aic.aic_pmu_model.NanoAicPmuModel.init'), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch('msmodel.aic.aic_pmu_model.NanoAicPmuModel.flush'), \
                mock.patch('msmodel.aic.aic_pmu_model.NanoAicPmuModel.finalize'):
            InfoConfReader()._info_json = {"devices": '0'}
            check = NanoAicCalculator({}, CONFIG)
            check._aic_data_list = [123]
            check.save()

    def test_ms_run_when_json_empty_then_return(self):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config",
                        return_value={'ai_core_profiling_mode': StrConstant.AIC_SAMPLE_BASED_MODE}), \
                mock.patch(NAMESPACE + '.PathManager.get_sample_json_path', return_value='test'):
            check = NanoAicCalculator({}, CONFIG)
            check.ms_run()

    def test_ms_run_when_normal_then_pass(self):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.PathManager.get_sample_json_path', return_value='test'), \
                mock.patch(NAMESPACE + '.NanoAicCalculator.init_params', return_value='test'), \
                mock.patch(NAMESPACE + '.NanoAicCalculator.calculate', return_value='test'), \
                mock.patch(NAMESPACE + '.NanoAicCalculator.save', return_value='test'):
            check = NanoAicCalculator({}, CONFIG)
            check.ms_run()

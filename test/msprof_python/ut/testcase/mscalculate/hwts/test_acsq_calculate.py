#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import unittest
from unittest import mock

from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.info_conf_reader import InfoConfReader
from constant.constant import CONFIG
from common_func.constant import Constant
from mscalculate.hwts.acsq_calculator import AcsqCalculator
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.struct_info.hwts_log import HwtsLogBean

NAMESPACE = 'mscalculate.hwts.acsq_calculator'


class TestAcsqCalculator(unittest.TestCase):
    file_list = {DataTag.STARS_LOG: ['stars_soc.data.0.slice_0']}

    def test_ms_run(self):
        ProfilingScene()._scene = "single_op"
        with mock.patch(NAMESPACE + '.AcsqCalculator.calculate'), \
                mock.patch(NAMESPACE + '.AcsqCalculator.save'):
            check = AcsqCalculator(self.file_list, CONFIG)
            check.ms_run()
        ProfilingScene()._scene = "single_op"
        with mock.patch(NAMESPACE + '.AcsqCalculator.calculate'), \
                mock.patch(NAMESPACE + '.AcsqCalculator.save', side_effect=RuntimeError):
            check = AcsqCalculator(self.file_list, CONFIG)
            check.ms_run()

    def test_parse_all_file(self):
        with mock.patch('msmodel.stars.acsq_task_model.AcsqTaskModel.init'), \
                mock.patch('msmodel.stars.acsq_task_model.AcsqTaskModel.drop_table'), \
                mock.patch('msmodel.stars.acsq_task_model.AcsqTaskModel.check_table', return_value=True), \
                mock.patch('common_func.db_manager.DBManager.fetch_all_data', return_value=[1]), \
                mock.patch('msmodel.stars.acsq_task_model.AcsqTaskModel.finalize'):
            InfoConfReader()._info_json = {"DeviceInfo": [
                {'id': 0, 'env_type': 3, 'ctrl_cpu_id': 'ARMv8_Cortex_A55', 'ctrl_cpu_core_num': 1,
                 'ctrl_cpu_endian_little': 1, 'ts_cpu_core_num': 0, 'ai_cpu_core_num': 1, 'ai_core_num': 2,
                 'ai_cpu_core_id': 1, 'ai_core_id': 0, 'aicpu_occupy_bitmap': 2, 'ctrl_cpu': '0', 'ai_cpu': '1',
                 'aiv_num': 0, 'hwts_frequency': '50', 'aic_frequency': '1500', 'aiv_frequency': '1500'}]}
            check = AcsqCalculator(self.file_list, CONFIG)
            check._log_data = [(0, 1, 61450515541, 61454924541, 'CPU_SQE'), (0, 3, 61479385166, 61480670819, 'CPU_SQE')]
            check._parse_all_file()

    def test_parse_by_iter(self):
        with mock.patch('msmodel.stars.acsq_task_model.AcsqTaskModel.init'), \
                mock.patch('msmodel.stars.acsq_task_model.AcsqTaskModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + '.MsprofIteration.get_step_iteration_time', return_value=[(1,)]), \
                mock.patch('msmodel.stars.acsq_task_model.AcsqTaskModel.drop_table'), \
                mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[(1,)]), \
                mock.patch('msmodel.stars.acsq_task_model.AcsqTaskModel.finalize'):
            InfoConfReader()._info_json = {"DeviceInfo": [
                {'id': 0, 'env_type': 3, 'ctrl_cpu_id': 'ARMv8_Cortex_A55', 'ctrl_cpu_core_num': 1,
                 'ctrl_cpu_endian_little': 1, 'ts_cpu_core_num': 0, 'ai_cpu_core_num': 1, 'ai_core_num': 2,
                 'ai_cpu_core_id': 1, 'ai_core_id': 0, 'aicpu_occupy_bitmap': 2, 'ctrl_cpu': '0', 'ai_cpu': '1',
                 'aiv_num': 0, 'hwts_frequency': '50', 'aic_frequency': '1500', 'aiv_frequency': '1500'}]}
            check = AcsqCalculator(self.file_list, CONFIG)
            check._log_data = [(0, 1, 61450515541, 61454924541, 'CPU_SQE'), (0, 3, 61479385166, 61480670819, 'CPU_SQE')]
            check._parse_by_iter()

    def test_save(self):
        with mock.patch('msmodel.stars.acsq_task_model.AcsqTaskModel.init'), \
                mock.patch(NAMESPACE + '.DBManager.clear_table'), \
                mock.patch('msmodel.stars.acsq_task_model.AcsqTaskModel.flush_task_time'), \
                mock.patch('msmodel.stars.acsq_task_model.AcsqTaskModel.finalize'):
            InfoConfReader()._info_json = {"DeviceInfo": [
                {'id': 0, 'env_type': 3, 'ctrl_cpu_id': 'ARMv8_Cortex_A55', 'ctrl_cpu_core_num': 1,
                 'ctrl_cpu_endian_little': 1, 'ts_cpu_core_num': 0, 'ai_cpu_core_num': 1, 'ai_core_num': 2,
                 'ai_cpu_core_id': 1, 'ai_core_id': 0, 'aicpu_occupy_bitmap': 2, 'ctrl_cpu': '0', 'ai_cpu': '1',
                 'aiv_num': 0, 'hwts_frequency': '50', 'aic_frequency': '1500', 'aiv_frequency': '1500'}]}
            check = AcsqCalculator(self.file_list, CONFIG)
            check._log_data = [(0, 1, 61450515541, 61454924541, 'CPU_SQE'), (0, 3, 61479385166, 61480670819, 'CPU_SQE')]
            check.save()

    def test_calculate(self):
        ProfilingScene()._scene = Constant.STEP_INFO
        with mock.patch(NAMESPACE + '.AcsqCalculator._parse_by_iter'):
            check = AcsqCalculator(self.file_list, CONFIG)
            check._log_data = [(0, 1, 61450515541, 61454924541, 'CPU_SQE'), (0, 3, 61479385166, 61480670819, 'CPU_SQE')]
            check.calculate()

        ProfilingScene()._scene = Constant.SINGLE_OP
        with mock.patch(NAMESPACE + '.AcsqCalculator._parse_all_file'):
            check = AcsqCalculator(self.file_list, CONFIG)
            check._log_data = [(0, 1, 61450515541, 61454924541, 'CPU_SQE'), (0, 3, 61479385166, 61480670819, 'CPU_SQE')]
            check.calculate()

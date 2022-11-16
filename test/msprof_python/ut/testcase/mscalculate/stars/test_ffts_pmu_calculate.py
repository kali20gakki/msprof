#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2022. All rights reserved.
"""
from unittest import TestCase
from unittest import mock

import pytest

from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.constant import Constant
from common_func.msprof_exception import ProfException
from constant.constant import CONFIG
from mscalculate.stars.ffts_pmu_calculate import FftsPmuCalculate
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'mscalculate.stars.ffts_pmu_calculate'


class TestFftsPmuCalculate(TestCase):

    def setUp(self):
        ProfilingScene().init('')

    def test_ms_run_sample_based(self):
        with mock.patch(NAMESPACE + '.generate_config',
                        return_value={"ai_core_profiling_mode": "sample-based", "aiv_profiling_mode": "sample-based"}):
            check = FftsPmuCalculate({DataTag.FFTS_PMU: ['ffts_profile.data.0.slice_0']}, CONFIG)
            check.ms_run()

    def test_ms_run_task_based(self):
        with mock.patch(NAMESPACE + '.generate_config',
                        return_value={"ai_core_profiling_mode": "task-based", "aiv_profiling_mode": "task-based"}), \
                mock.patch(NAMESPACE + '.FftsPmuCalculate.calculate'), \
                mock.patch(NAMESPACE + '.FftsPmuCalculate.save'):
            check = FftsPmuCalculate({DataTag.FFTS_PMU: ['ffts_profile.data.0.slice_0']}, CONFIG)
            check.ms_run()

    def test_calculate_by_iter_no_table(self):
        with mock.patch(NAMESPACE + '.Utils.get_scene', return_value=Constant.STEP_INFO), \
                mock.patch("os.path.exists", return_value=True), \
                mock.patch("os.remove"), \
                mock.patch("msmodel.interface.base_model.BaseModel.init"), \
                mock.patch("msmodel.interface.base_model.BaseModel.finalize"), \
                mock.patch("msmodel.interface.base_model.BaseModel.create_table"), \
                mock.patch(NAMESPACE + '.HwtsIterModel.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.HwtsIterModel.check_table', return_value=False):
            check = FftsPmuCalculate({DataTag.FFTS_PMU: ['ffts_profile.data.0.slice_0']}, CONFIG)
            check.calculate()

    def test_calculate_by_iter_table_exist_and_save(self):
        with mock.patch(NAMESPACE + '.Utils.get_scene', return_value=Constant.STEP_INFO), \
                mock.patch("os.path.exists", return_value=True), \
                mock.patch("os.remove"), \
                mock.patch("msmodel.interface.base_model.BaseModel.init"), \
                mock.patch("msmodel.interface.base_model.BaseModel.finalize"), \
                mock.patch("msmodel.interface.base_model.BaseModel.create_table"), \
                mock.patch(NAMESPACE + '.HwtsIterModel.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.HwtsIterModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + '.MsprofIteration.get_iter_id_by_index_id', return_value=[0, 1]), \
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
            check = FftsPmuCalculate({DataTag.FFTS_PMU: ['ffts_profile.data.0.slice_0']}, CONFIG)
            check.calculate()
            check.save()

    def test_calculate_all_file(self):
        with mock.patch(NAMESPACE + '.Utils.get_scene', return_value=Constant.SINGLE_OP), \
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
            check = FftsPmuCalculate({DataTag.FFTS_PMU: ['ffts_profile.data.0.slice_0']}, CONFIG)
            check.calculate()

    def test_save_no_data(self):
        with mock.patch(NAMESPACE + '.logging.warning'):
            check = FftsPmuCalculate({DataTag.FFTS_PMU: ['ffts_profile.data.0.slice_0']}, CONFIG)
            check.save()

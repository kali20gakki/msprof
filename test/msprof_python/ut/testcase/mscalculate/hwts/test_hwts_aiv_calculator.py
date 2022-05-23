#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import unittest
from unittest import mock

from analyzer.scene_base.profiling_scene import ProfilingScene
from constant.constant import CONFIG
from mscalculate.hwts.hwts_aiv_calculator import HwtsAivCalculator
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'mscalculate.hwts.hwts_aiv_calculator'


class TestHwtsAivCalculator(unittest.TestCase):
    file_list = {DataTag.AIV: ['aiVectorCore.data.0.slice_0']}

    def test_ms_run(self):

        with mock.patch(NAMESPACE + '.HwtsAivCalculator._parse_all_file'), \
             mock.patch('common_func.msprof_iteration' + '.MsprofIteration.get_iteration_end_dict', return_value=[]), \
             mock.patch('common_func.msprof_iteration' + '.MsprofIteration.get_op_iteration_dict', return_value=[]), \
             mock.patch('common_func.msprof_iteration' + '.MsprofIteration.get_iteration_dict', return_value=[]), \
             mock.patch(NAMESPACE + '.HwtsAivCalculator.save'):
            ProfilingScene()._scene = "single_op"
            check = HwtsAivCalculator(self.file_list, CONFIG)
            check.ms_run()

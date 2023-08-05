#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
import os
import unittest
from unittest import mock

from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.constant import Constant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.platform.chip_manager import ChipManager
from mscalculate.ascend_task.ascend_task import TopDownTask
from mscalculate.ascend_task.ascend_task_calculator import AscendTaskCalculator
from profiling_bean.db_dto.step_trace_dto import IterationRange
from profiling_bean.prof_enum.chip_model import ChipModel

NAMESPACE = 'mscalculate.ascend_task.ascend_task_calculator'


class TestAscendTaskCalculator(unittest.TestCase):
    DIR_PATH = os.path.join(os.path.dirname(__file__), "DT_DeviceTaskCollector")
    PROF_DIR = os.path.join(DIR_PATH, 'PROF1')
    calculator = AscendTaskCalculator({}, {StrConstant.PARAM_ITER_ID: IterationRange(1, 1, 1)})
    info_reader = InfoConfReader()
    info_reader._info_json = {'platform_version': "5"}

    def test__collect_ascend_tasks_in_graph_scene(self):
        scene = ProfilingScene()
        scene._scene = Constant.STEP_INFO
        with mock.patch(NAMESPACE + ".AscendTaskGenerator.run",
                        return_value=[TopDownTask(1, 1, 1, 1, 1, 1, 1000, 1000, "", "")]):
            self.assertEqual(len(self.calculator._collect_ascend_tasks()), 1)
        scene._scene = None

    def test__collect_ascend_tasks_in_operator_scene(self):
        scene = ProfilingScene()
        scene._scene = Constant.SINGLE_OP
        with mock.patch(NAMESPACE + ".AscendTaskGenerator.run",
                        return_value=[TopDownTask(1, 1, 1, 1, 1, 1, 1000, 1000, "", ""),
                                      TopDownTask(1, 2, 1, 1, 1, 1, 1000, 1000, "", "")]):
            self.assertEqual(len(self.calculator._collect_ascend_tasks()), 2)
        scene._scene = None

    def test__judge_calculate_again_should_return_true_when_in_graph_scene(self):
        scene = ProfilingScene()
        scene._scene = Constant.STEP_INFO
        self.assertTrue(self.calculator._judge_calculate_again())
        scene._scene = None

    def test__judge_calculate_again_should_return_true_when_no_ascend_task_db_in_op_scene(self):
        scene = ProfilingScene()
        scene._scene = Constant.SINGLE_OP
        with mock.patch(NAMESPACE + ".DBManager.check_tables_in_db", return_value=False):
            self.assertTrue(self.calculator._judge_calculate_again())
        scene._scene = None

    def test__judge_calculate_again_should_return_false_when_exist_ascend_task_db_in_op_scene(self):
        scene = ProfilingScene()
        scene._scene = Constant.SINGLE_OP
        with mock.patch(NAMESPACE + ".DBManager.check_tables_in_db", return_value=True):
            self.assertFalse(self.calculator._judge_calculate_again())
        scene._scene = None

    def test__judge_calculate_again_should_return_false_when_sample_based_in_stars(self):
        info = InfoConfReader()
        info._sample_json = {
            "ai_core_profiling_mode": 'sample-based',
            'platform_version': "5"
        }
        chip_m = ChipManager()
        chip_m.chip_id = ChipModel.CHIP_V1_1_1
        self.assertFalse(self.calculator._judge_calculate_again())
        info._sample_json = {}
        chip_m.chip_id = ChipModel.CHIP_V1_1_0

    def test__save(self):
        data = [TopDownTask(1, 1, 1, 1, 1, 1, 1000, 1000, "", "")]
        with mock.patch(NAMESPACE + ".AscendTaskModel.init"), \
                mock.patch(NAMESPACE + ".AscendTaskModel.drop_table"), \
                mock.patch(NAMESPACE + ".AscendTaskModel.create_table"), \
                mock.patch(NAMESPACE + ".AscendTaskModel.insert_data_to_db"), \
                mock.patch(NAMESPACE + ".AscendTaskModel.finalize"):
            self.calculator._save(data)

    def test_ms_run(self):
        with mock.patch(NAMESPACE + ".AscendTaskCalculator._collect_ascend_tasks", return_value=[]):
            with mock.patch(NAMESPACE + ".AscendTaskCalculator._judge_calculate_again", return_value=False):
                self.calculator.ms_run()
            with mock.patch(NAMESPACE + ".AscendTaskCalculator._judge_calculate_again", return_value=True):
                self.calculator.ms_run()

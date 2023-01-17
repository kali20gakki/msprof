#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

from analyzer.create_ai_core_op_summary_db import ParseAiCoreOpSummary
from analyzer.create_op_counter_db import MergeOPCounter
from analyzer.scene_base.op_counter_op_scene import OpCounterOpScene
from analyzer.scene_base.op_summary_op_scene import OpSummaryOpScene
from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.constant import Constant
from mscalculate.runtime.calculate_op_task_scheduler import CalculateOpTaskScheduler
from mscalculate.runtime.calculate_task_scheduler import CalculateTaskScheduler


class DataAnalysisFactory:
    """
    analysis profiling data with different scene
    """
    EXPORT_CLASS_NEED_MAP = {
        Constant.STEP_INFO: [CalculateTaskScheduler, ParseAiCoreOpSummary, MergeOPCounter],
        Constant.MIX_OP_AND_GRAPH: [CalculateTaskScheduler, ParseAiCoreOpSummary, MergeOPCounter],
        Constant.SINGLE_OP: [CalculateOpTaskScheduler, OpSummaryOpScene, OpCounterOpScene]
    }

    def __init__(self: any, sample_json: dict) -> None:
        self.sample_json = sample_json
        self.project_path = sample_json.get("result_dir")
        self.scene = None

    def parse_data(self: any) -> None:
        """
        parse profiling data
        :return: None
        """
        if self.scene is None:
            return
        for class_obj in self.EXPORT_CLASS_NEED_MAP.get(self.scene):
            class_obj(self.sample_json).run()

    def run(self: any) -> None:
        """
        run and analysis for pre-export
        :return: None
        """
        self.scene = ProfilingScene().get_scene()
        self.parse_data()

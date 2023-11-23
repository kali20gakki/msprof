#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023. All rights reserved.

import json
import logging
import os
from enum import Enum

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.path_manager import PathManager
from common_func.section_calculator import SectionCalculator
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from msmodel.hccl.hccl_model import HcclViewModel
from msmodel.stars.op_summary_model import OpSummaryModel


class OverlapType(Enum):
    COMPUTE_TIME = 0
    COMMUNICATION_TIME = 1
    COMMUNICATION_NOT_OVERLAPPED = 2
    FREE_TIME = 3


class PipelineOverlapViewer:
    OVERLAP_TYPE_NAME = {
        "COMPUTE_TIME": "Computing",
        "COMMUNICATION_TIME": "Communication",
        "COMMUNICATION_NOT_OVERLAPPED": "Communication(Not Overlapped)",
        "FREE_TIME": "Free"
    }

    def __init__(self, configs: dict, params: dict):
        self._configs = configs
        self._params = params
        self._project_path = params.get(StrConstant.PARAM_RESULT_DIR)
        self._pid = InfoConfReader().get_json_pid_data()

    def get_timeline_data(self):
        result = []
        compute_data = []
        if os.path.exists(PathManager.get_db_path(self._project_path, DBNameConstant.DB_AICORE_OP_SUMMARY)):
            sample_config = {
                'result_dir': self._project_path,
                'iter_id': Constant.DEFAULT_INVALID_VALUE,
                'model_id': Constant.DEFAULT_INVALID_VALUE
            }
            with OpSummaryModel(sample_config) as _model:
                compute_data = _model.get_operator_data_by_task_type()
        compute_data = SectionCalculator.merge_continuous_intervals(compute_data)
        result.extend(self._format_timeline_data(OverlapType.COMPUTE_TIME, data) for data in compute_data)

        communication_data = []
        if os.path.exists(PathManager.get_db_path(self._project_path, DBNameConstant.DB_HCCL_SINGLE_DEVICE)):
            with HcclViewModel(self._project_path, DBNameConstant.DB_HCCL_SINGLE_DEVICE,
                               [DBNameConstant.TABLE_HCCL_SINGLE_DEVICE]) as _model:
                if _model.check_table():
                    communication_data = _model.get_hccl_op_time_section()
        communication_data = SectionCalculator.merge_continuous_intervals(communication_data)
        result.extend(
            self._format_timeline_data(OverlapType.COMMUNICATION_TIME, data)
            for data in communication_data
        )

        if not compute_data and not communication_data:
            logging.warning("Both task data and hccl data are missing, no need to calculate the overlap.")
            return ""
        pure_communication_section, free_time_section = SectionCalculator.compute_pipeline_overlap(communication_data,
                                                                                                   compute_data)
        result.extend(
            self._format_timeline_data(OverlapType.COMMUNICATION_NOT_OVERLAPPED, data)
            for data in pure_communication_section
        )
        result.extend(self._format_timeline_data(OverlapType.FREE_TIME, data) for data in free_time_section)
        _trace = TraceViewManager.time_graph_trace(TraceViewHeaderConstant.TASK_TIME_GRAPH_HEAD, result)
        _trace.extend(TraceViewManager.metadata_event([["process_name", self._pid, InfoConfReader().get_json_tid_data(),
                                                        TraceViewHeaderConstant.PROCESS_OVERLAP_ANALYSE]]))
        _trace.extend(
            TraceViewManager.metadata_event(
                [["thread_name", self._pid, overlap_type.value, self.OVERLAP_TYPE_NAME.get(overlap_type.name)]
                 for overlap_type in OverlapType]
            )
        )
        return _trace

    def _format_timeline_data(self, overlap_type, data):
        return [self.OVERLAP_TYPE_NAME.get(overlap_type.name), self._pid, overlap_type.value,
                InfoConfReader().trans_into_local_time(data.start_time),
                (data.end_time - data.start_time) / NumberConstant.NS_TO_US]
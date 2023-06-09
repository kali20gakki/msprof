#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.

from collections import namedtuple


class TraceViewHeaderConstant:
    """
    trace view header constant class
    """
    TRACE_HEADER_TS = 'ts'
    TRACE_HEADER_DURATION = 'dur'
    # column graph format
    COLUMN_GRAPH_HEAD_LEAST = ['name', 'ts', 'pid', 'tid', 'args']  # name, ts, pid, tid, args is required
    # timeline graph format
    TIME_GRAPH_HEAD_LEAST = ['name', 'pid', 'ts', 'dur']  # name, pid, ts, dur is required
    TASK_TIME_GRAPH_HEAD = ['name', 'pid', 'tid', 'ts', 'dur']
    TOP_DOWN_TIME_GRAPH_HEAD = ['name', 'pid', 'tid', 'ts', 'dur', 'args']
    GRPC_TIME_GRAPH_HEAD = ['name', 'pid', 'tid', 'ts', 'dur', 'args', 'cat']

    # meta data head format
    METADATA_HEAD = ["name", "pid", "tid", "args"]

    # pid value when it not set
    DEFAULT_PID_VALUE = 0

    # tid value when it not set
    DEFAULT_TID_VALUE = 0

    # process name
    PROCESS_RUNTIME = "Runtime"
    PROCESS_AI_CORE_UTILIZATION = "AI Core Utilization"
    PROCESS_AI_STACK_TIME = "AI Stack Time"
    PROCESS_ACL = "AscendCL"
    PROCESS_AI_CPU = "AI CPU"
    PROCESS_ALL_REDUCE = "All Reduce"
    PROCESS_GE = "GE"
    PROCESS_TASK = "Task Scheduler"
    PROCESS_STEP_TRACE = "Step Trace"
    PROCESS_TRAINING_TRACE = "Training Trace"
    PROCESS_PCIE = "Pcie"
    PROCESS_MSPROFTX = "MsprofTx"
    PROCESS_SUBTASK = 'Subtask Time'
    PROCESS_THREAD_TASK = 'Thread Task Time'
    PROCESS_PTA = "PTA"
    PROCESS_OVERLAP_ANALYSE = "Overlap Analysis"
    PROCESS_API = "Api"
    PROCESS_EVENT = "Event"

    # trace general layer
    GENERAL_LAYER_CPU = "CPU"
    GENERAL_LAYER_NPU = "NPU"

    # trace component layer
    COMPONENT_LAYER_FRAMEWORK = "PID Name"
    COMPONENT_LAYER_PTA = "PTA"
    COMPONENT_LAYER_CANN = "CANN"
    COMPONENT_LAYER_ASCEND_HW = "Ascend Hardware"

    # filtering msprof timeline trace
    MSPROF_TIMELINE_FILTER_LIST = (PROCESS_ALL_REDUCE, PROCESS_AI_CPU)

    # component_layer_sort
    LAYER_FRAMEWORK_SORT = 0
    LAYER_PTA_SORT = 1
    LAYER_CANN_SORT = 2
    LAYER_ASCEND_HW_SORT = 3

    # namedtuple configuration of LayerInfo
    LayerInfo = namedtuple('LayerInfo', ['component_layer', 'general_layer', 'sort_index'])

    # 【msprof.json】 timeline layer info map
    LAYER_INFO_MAP = {
        PROCESS_MSPROFTX: LayerInfo(COMPONENT_LAYER_FRAMEWORK, GENERAL_LAYER_CPU, LAYER_FRAMEWORK_SORT),
        PROCESS_PTA: LayerInfo(COMPONENT_LAYER_PTA, GENERAL_LAYER_CPU, LAYER_PTA_SORT),
        PROCESS_ACL: LayerInfo(COMPONENT_LAYER_CANN, GENERAL_LAYER_CPU, LAYER_CANN_SORT),
        PROCESS_GE: LayerInfo(COMPONENT_LAYER_CANN, GENERAL_LAYER_CPU, LAYER_CANN_SORT),
        PROCESS_RUNTIME: LayerInfo(COMPONENT_LAYER_CANN, GENERAL_LAYER_CPU, LAYER_CANN_SORT),
        PROCESS_TASK: LayerInfo(COMPONENT_LAYER_ASCEND_HW, GENERAL_LAYER_NPU,
                                LAYER_ASCEND_HW_SORT),
        PROCESS_STEP_TRACE: LayerInfo(COMPONENT_LAYER_ASCEND_HW, GENERAL_LAYER_NPU,
                                      LAYER_ASCEND_HW_SORT),
        PROCESS_API: LayerInfo(COMPONENT_LAYER_CANN, GENERAL_LAYER_CPU, LAYER_CANN_SORT),
        PROCESS_EVENT: LayerInfo(COMPONENT_LAYER_CANN, GENERAL_LAYER_CPU, LAYER_CANN_SORT),
    }

    @classmethod
    def update_layer_info_map(cls: any, process_name: str) -> None:
        """
        update LAYER_INFO_MAP based on process_name
        """
        if process_name is not None and process_name != "":
            cls.COMPONENT_LAYER_FRAMEWORK = process_name
            cls.LAYER_INFO_MAP.update(
                {cls.PROCESS_MSPROFTX: cls.LayerInfo(cls.COMPONENT_LAYER_FRAMEWORK,
                                                     cls.GENERAL_LAYER_CPU, cls.LAYER_FRAMEWORK_SORT)})

    def get_trace_view_header_constant_class_name(self: any) -> any:
        """
        get trace view header constant class name
        """
        return self.__class__.__name__

    def get_trace_view_header_constant_class_member(self: any) -> any:
        """
        get trace view header constant class member num
        """
        return self.__dict__

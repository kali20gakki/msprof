#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.


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
    PROCESS_TRAINING_TRACE = "Training Trace"
    PROCESS_PCIE = "Pcie"
    PEOCESS_MSPROFTX = "MsprofTx"
    PEOCESS_ACSQ = "ACSQ"

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

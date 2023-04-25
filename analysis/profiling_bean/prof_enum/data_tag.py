#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.

from enum import Enum
from enum import unique


@unique
class DataTag(Enum):
    """
    Define the tag for profiling data
    """
    ACL = 0
    ACL_HASH = 1
    GE_TASK = 2
    GE_MODEL_TIME = 3
    GE_MODEL_LOAD = 4
    GE_STEP = 5
    RUNTIME_API = 6
    RUNTIME_TRACK = 7
    TS_TRACK = 8
    HWTS = 9
    AI_CORE = 10
    AI_CPU = 11
    TRAINING_TRACE = 12
    DATA_PROCESS = 13
    STARS_LOG = 14
    FFTS_PMU = 15
    AIV = 16
    DVPP = 17
    NIC = 18
    ROCE = 19
    TSCPU = 20
    CTRLCPU = 21
    AICPU = 22
    DDR = 23
    SYS_MEM = 24
    PID_MEM = 25
    SYS_USAGE = 26
    PID_USAGE = 27
    PCIE = 28
    LLC = 29
    HBM = 30
    HCCS = 31
    TS_TRACK_AIV = 32
    L2CACHE = 33
    HWTS_AIV = 34
    HCCL = 35
    MSPROFTX = 36
    GE_TENSOR = 37
    GE_FUSION_OP_INFO = 38
    GE_SESSION = 39
    GE_HASH = 40
    GE_HOST = 41
    SOC_PROFILER = 42
    HELPER_MODEL_WITH_Q = 43
    BIU_PERF = 44
    DATA_QUEUE = 45
    HOST_QUEUE = 46
    PARALLEL_STRATEGY = 47
    MSPROFTX_TORCH = 48
    MSPROFTX_CANN = 49
    NPU_MEM = 50
    MSPROFTX_PIPELINE = 51
    FREQ = 52
    API = 55
    HASH_DICT = 53
    EVENT = 54
    TASK_TRACK = 56
    MEMCPY_INFO = 57
    GRAPH_ADD_INFO = 60
    TENSOR_ADD_INFO = 61
    BASIC_ADD_INFO = 62
    FUSION_ADD_INFO = 63


@unique
class AclApiTag(Enum):
    """
    Define the tag for acl api type
    """
    ACL_OP = 1
    ACL_MODEL = 2
    ACL_RTS = 3
    ACL_OTHERS = 4

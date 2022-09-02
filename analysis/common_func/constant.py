#!/usr/bin/python3
# coding=utf-8
"""
This script is used to provid profiling constants
Copyright Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
"""
import os
import stat
from common_func.ms_constant.number_constant import NumberConstant


class PmuMetricCalculate:
    """
    class for pmu metric calculate
    """

    @staticmethod
    def pmu_metric_calculate_with_freq(float_bit: float, pip_size: float, scalar: float, *base_info: any) -> float:
        """
        calculate pmu metric with freq
        :param float_bit: 1.0
        :param pip_size: pip size
        :param scalar: scalar num
        :param base_info: pmu, task_cyc, freq
        :return: metric value
        """
        pmu, task_cyc, freq = base_info
        if NumberConstant.is_zero(task_cyc) or NumberConstant.is_zero(freq):
            return 0
        return float_bit * pmu * pip_size * scalar / (task_cyc / freq) / 8589934592.0

    @staticmethod
    def pmu_metric_calculate_without_freq(float_bit: float, *base_info: any) -> float:
        """
        calculate pmu metric without freq
        :param float_bit: 1.0
        :param base_info: pmu, task_cyc
        :return: metric value
        """
        pmu, task_cyc = base_info
        if NumberConstant.is_zero(task_cyc):
            return 0
        return float_bit * pmu / task_cyc


class Constant:
    """
    constant class
    """
    TRACE_BACK_SWITCH = True
    L2_CACHE_EVENTS = [
        "0x59", "0x5b", "0x5c", "0x62", "0x6a", "0x6c", "0x71",
        "0x74", "0x77", "0x78", "0x79", "0x7c", "0x7d", "0x7e"
    ]

    INVALID_INDEX = -1

    AI_CORE_CALCULATION_FORMULA = {
        "mac_fp16_ratio": lambda pmu, task_cyc:
        PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc),
        "mac_int8_ratio": lambda pmu, task_cyc:
        PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc),
        "vec_fp32_ratio": lambda pmu, task_cyc:
        PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc),
        "vec_fp16_128lane_ratio": lambda pmu, task_cyc:
        PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc),
        "vec_fp16_64lane_ratio": lambda pmu, task_cyc:
        PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc),
        "vec_fp16_ratio": lambda pmu1, pmu2, task_cyc: 1.0 * (pmu1 + pmu2) / task_cyc,
        "vec_int32_ratio": lambda pmu, task_cyc:
        PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc),
        "vec_misc_ratio": lambda pmu, task_cyc:
        PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc),
        "vec_ratio": lambda pmu, task_cyc:
        PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc),
        "mac_ratio": lambda pmu, task_cyc:
        PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc),
        "scalar_ratio": lambda pmu, task_cyc:
        PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc),
        "mte1_ratio": lambda pmu, task_cyc:
        PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc),
        "mte2_ratio": lambda pmu, task_cyc:
        PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc),
        "mte3_ratio": lambda pmu, task_cyc:
        PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc),
        "icache_miss_rate": lambda pmu, task_cyc:
        PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc),
        "icache_req_ratio": lambda pmu, task_cyc:
        PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc),
        "scalar_waitflag_ratio": lambda pmu, task_cyc:
        PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc),
        "cube_waitflag_ratio": lambda pmu, task_cyc:
        PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc),
        "vector_waitflag_ratio": lambda pmu, task_cyc:
        PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc),
        "mte1_waitflag_ratio": lambda pmu, task_cyc:
        PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc),
        "mte2_waitflag_ratio": lambda pmu, task_cyc:
        PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc),
        "mte3_waitflag_ratio": lambda pmu, task_cyc:
        PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc),
        "scalar_ld_ratio": lambda pmu, task_cyc:
        PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc),
        "scalar_st_ratio": lambda pmu, task_cyc:
        PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc),
        "vec_bankgroup_cflt_ratio": lambda pmu, task_cyc:
        PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc),
        "vec_bank_cflt_ratio": lambda pmu, task_cyc:
        PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc),
        "vec_resc_cflt_ratio": lambda pmu, task_cyc:
        PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc),
        "mte1_iq_full_ratio": lambda pmu, task_cyc:
        PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc),
        "mte2_iq_full_ratio": lambda pmu, task_cyc:
        PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc),
        "mte3_iq_full_ratio": lambda pmu, task_cyc:
        PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc),
        "cube_iq_full_ratio": lambda pmu, task_cyc:
        PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc),
        "vec_iq_full_ratio": lambda pmu, task_cyc:
        PmuMetricCalculate.pmu_metric_calculate_without_freq(1.0, pmu, task_cyc),
        "ub_read_bw(GB/s)": lambda pmu, task_cyc, freq:
        PmuMetricCalculate.pmu_metric_calculate_with_freq(1.0, 256.0, 4.0, pmu, task_cyc, freq),
        "ub_write_bw(GB/s)": lambda pmu, task_cyc, freq:
        PmuMetricCalculate.pmu_metric_calculate_with_freq(1.0, 256.0, 4.0, pmu, task_cyc, freq),
        "l1_read_bw(GB/s)": lambda pmu, task_cyc, freq:
        PmuMetricCalculate.pmu_metric_calculate_with_freq(1.0, 256.0, 16.0, pmu, task_cyc, freq),
        "l1_write_bw(GB/s)": lambda pmu, task_cyc, freq:
        PmuMetricCalculate.pmu_metric_calculate_with_freq(1.0, 128.0, 8.0, pmu, task_cyc, freq),
        "l2_read_bw(GB/s)": lambda pmu, task_cyc, freq:
        PmuMetricCalculate.pmu_metric_calculate_with_freq(1.0, 256.0, 8.0, pmu, task_cyc, freq),
        "l2_write_bw(GB/s)": lambda pmu, task_cyc, freq:
        PmuMetricCalculate.pmu_metric_calculate_with_freq(1.0, 256.0, 8.0, pmu, task_cyc, freq),
        "main_mem_read_bw(GB/s)": lambda pmu, task_cyc, freq:
        PmuMetricCalculate.pmu_metric_calculate_with_freq(1.0, 256.0, 8.0, pmu, task_cyc, freq),
        "main_mem_write_bw(GB/s)": lambda pmu, task_cyc, freq:
        PmuMetricCalculate.pmu_metric_calculate_with_freq(1.0, 256.0, 8.0, pmu, task_cyc, freq),
        "l0a_read_bw(GB/s)": lambda pmu, task_cyc, freq:
        PmuMetricCalculate.pmu_metric_calculate_with_freq(1.0, 256.0, 16.0, pmu, task_cyc, freq),
        "l0a_write_bw(GB/s)": lambda pmu, task_cyc, freq:
        PmuMetricCalculate.pmu_metric_calculate_with_freq(1.0, 256.0, 16.0, pmu, task_cyc, freq),
        "l0b_read_bw(GB/s)": lambda pmu, task_cyc, freq:
        PmuMetricCalculate.pmu_metric_calculate_with_freq(1.0, 256.0, 16.0, pmu, task_cyc, freq),
        "l0b_write_bw(GB/s)": lambda pmu, task_cyc, freq:
        PmuMetricCalculate.pmu_metric_calculate_with_freq(1.0, 256.0, 8.0, pmu, task_cyc, freq),
        "l0c_read_bw(GB/s)": lambda pmu, task_cyc, freq:
        PmuMetricCalculate.pmu_metric_calculate_with_freq(1.0, 256.0, 8.0, pmu, task_cyc, freq),
        "l0c_write_bw(GB/s)": lambda pmu, task_cyc, freq:
        PmuMetricCalculate.pmu_metric_calculate_with_freq(1.0, 256.0, 8.0, pmu, task_cyc, freq),
        "l0c_read_bw_cube(GB/s)": lambda pmu, task_cyc, freq:
        PmuMetricCalculate.pmu_metric_calculate_with_freq(1.0, 256.0, 32.0, pmu, task_cyc, freq),
        "l0c_write_bw_cube(GB/s)": lambda pmu, task_cyc, freq:
        PmuMetricCalculate.pmu_metric_calculate_with_freq(1.0, 256.0, 32.0, pmu, task_cyc, freq),
        "ub_read_bw_vector(GB/s)": lambda pmu, task_cyc, freq:
        PmuMetricCalculate.pmu_metric_calculate_with_freq(1.0, 256.0, 16.0, pmu, task_cyc, freq),
        "ub_write_bw_vector(GB/s)": lambda pmu, task_cyc, freq:
        PmuMetricCalculate.pmu_metric_calculate_with_freq(1.0, 256.0, 8.0, pmu, task_cyc, freq),
        "ub_read_bw_scalar(GB/s)": lambda pmu, task_cyc, freq:
        PmuMetricCalculate.pmu_metric_calculate_with_freq(1.0, 256.0, 32.0, pmu, task_cyc, freq),
        "ub_write_bw_scalar(GB/s)": lambda pmu, task_cyc, freq:
        PmuMetricCalculate.pmu_metric_calculate_with_freq(1.0, 256.0, 32.0, pmu, task_cyc, freq),
    }

    AICORE_PIPE_LIST = ["vec_time", "mac_time", "scalar_time", "mte1_time", "mte2_time", "mte3_time"]

    AICORE_METRICS_LIST = {
        "ArithmeticUtilization": "mac_fp16_ratio,mac_int8_ratio,vec_fp32_ratio,"
                                 "vec_fp16_ratio,vec_int32_ratio,vec_misc_ratio",
        "PipeUtilization": "vec_ratio,mac_ratio,scalar_ratio,mte1_ratio,"
                           "mte2_ratio,mte3_ratio,icache_miss_rate",
        "Memory": "ub_read_bw(GB/s),ub_write_bw(GB/s),l1_read_bw(GB/s),"
                  "l1_write_bw(GB/s),l2_read_bw(GB/s),l2_write_bw(GB/s),"
                  "main_mem_read_bw(GB/s),main_mem_write_bw(GB/s)",
        "MemoryL0": "l0a_read_bw(GB/s),l0a_write_bw(GB/s),l0b_read_bw(GB/s),"
                    "l0b_write_bw(GB/s),l0c_read_bw(GB/s),l0c_write_bw(GB/s),"
                    "l0c_read_bw_cube(GB/s),l0c_write_bw_cube(GB/s)",
        "ResourceConflictRatio": "vec_bankgroup_cflt_ratio,vec_bank_cflt_ratio,"
                                 "vec_resc_cflt_ratio",
        "MemoryUB": "ub_read_bw_mte(GB/s),ub_write_bw_mte(GB/s),"
                    "ub_read_bw_vector(GB/s),ub_write_bw_vector(GB/s),"
                    "ub_read_bw_scalar(GB/s),ub_write_bw_scalar(GB/s)"
    }

    # add default limit for reader buffer size ->8196  * 1024 Byte
    MAX_READ_LINE_BYTES = 8196 * 1024
    MAX_READ_FILE_BYTES = 64 * 1024 * 1024

    DEFAULT_START = 1

    PLATFORM_VERSION = "platform_version"
    CHIP_V1_1_0 = "0"
    CHIP_V2_1_0 = "1"
    CHIP_V3_1_0 = "2"
    CHIP_V3_2_0 = "3"
    CHIP_V3_3_0 = "4"
    CHIP_V4_1_0 = "5"

    MIX_OP_AND_GRAPH = "mix_operator_and_graph"
    STEP_INFO = "step_info"
    TRAIN = "train"
    SINGLE_OP = "single_op"
    DONE_TAG = ".done"
    ZIP_TAG = ".zip"
    COMPLETE_TAG = ".complete"
    FOLDER_MASK = 0o750
    LINE_LEN = 3
    SAMPLE_FILE = "sample.json"
    DEFAULT_REPLAY = 0
    DEFAULT_DEVICE = 0
    DEFAULT_COUNT = 0
    KILOBYTE = 1024.0
    TIME_RATE = 1000000000.0
    THOUSAND = 1000
    BYTE_NS_TO_MB_S = TIME_RATE / KILOBYTE / KILOBYTE
    BYTE_US_TO_MB_S = TIME_RATE / THOUSAND / KILOBYTE / KILOBYTE
    L2_CACHE_ITEM = 8
    HEX_NUMBER = 16
    DVPP_TYPE_NAME = ['VDEC', 'JPEGD', 'PNGD', 'JPEGE', 'VPC']
    FILTER_DIRS = [".profiler", "HCCL_PROF", "timeline", "query", 'sqlite', 'log']
    NA = 'N/A'
    TASK_TYPE_OTHER = "Other"
    TASK_TYPE_AI_CORE = "AI_CORE"
    TASK_TYPE_AI_CPU = "AI_CPU"
    TASK_TYPE_AIV = "AI_VECTOR_CORE"
    DATA_PROCESS_AI_CPU = "AICPU"
    DATA_PROCESS_DP = "DP"
    DATA_QUEUE = "AICPUMI"
    WRITE_MODES = stat.S_IWUSR | stat.S_IRUSR | stat.S_IRGRP
    WRITE_FLAGS = os.O_WRONLY | os.O_CREAT | os.O_TRUNC

    # hash dict flag
    NO_HASH_DICT_FLAG = 1
    HASH_DICT_FLAG = 0

    # default value
    DEFAULT_VALUE = 0
    DEFAULT_INVALID_VALUE = -1
    DEFAULT_TURE_VALUE = 1
    DEFAULT_FALSE_VALUE = 0

    # ge timeline
    GE_TIMELINE_MODEL_ID_INDEX = 0
    GE_TIMELINE_MODEL_ID_INDEX_NAME_INDEX = 6
    GE_TIMELINE_TID_INDEX = 8

    GE_OP_MODEL_ID = 4294967295

    # hccl
    TYPE_RDMA = "RDMA"
    TYPE_SDMA = "SDMA"
    ILLEGAL_RANK = 4294967295
    LINK_TYPE_LIST = [TYPE_SDMA, TYPE_RDMA]

    def get_constant_class_name(self: any) -> any:
        """
        get constant class name
        """
        return self.__class__.__name__

    def get_constant_class_member(self: any) -> any:
        """
        get constant class member num
        """
        return self.__dict__

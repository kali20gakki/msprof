#!/usr/bin/python3
"""
This script is used to provid profiling constants
Copyright Huawei Technologies Co., Ltd. 2021. All rights reserved.
"""
import struct


class StructFmt:
    """
    struct format constant
    """

    BYTE_ORDER_CHAR = '='
    AICORE_FMT_SIZE = 128
    AICORE_FMT = 'BBHHHII10Q8I'
    AICORE_SAMPLE_FMT_SIZE = 96
    AICORE_SAMPLE_FMT = 'BBHHH8QQQBB6B'
    AIV_SAMPLE_FMT_SIZE = 96
    AIV_SAMPLE_FMT = 'BBHHH8QQQBB6B'
    AIV_FMT_SIZE = 128
    AIV_FMT = 'BBHHHIIqqqqqqqqqqIIIIIIII'
    TSCPU_FMT_SIZE = 304
    TSCPU_FMT = "LL20QLLQQ14Q"
    MDC_TSCPU_FMT_SIZE = 184
    MDC_TSCPU_FMT = "LL20LLLQ10Q"
    DDR_FMT_SIZE = 20
    DDR_FMT = "LLLLL"
    PCIE_FMT = "Q22I"
    PCIE_FMT_SIZE = 96
    HBM_FMT = "QQII"
    HBM_FMT_SIZE = 24
    LLC_FMT = 'QQII'
    LLC_FMT_SIZE = 24
    STEP_TRACE_FMT = "BBHLQQQHHHH"
    STEP_TRACE_FMT_SIZE = 40
    ACL_FMT = "HHIQQII64s4Q"
    ACL_FMT_SIZE = 128
    RTS_TRACK_FMT = "HHIQ32sHHHHQ"
    RTS_TRACK_FMT_SIZE = 64
    L2_CACHE_STRUCT_FMT = "HHHH8Q"
    L2_CACHE_DATA_SIZE = 72

    # binary data struct fmt
    API_CALL_FMT = "QQLLLLLL"
    TIME_LINE_FMT = "BBHLHHHHQLL"
    TS_MEMCPY_FMT = "BBHLQHHBBHQQ"
    EVENT_COUNT_FMT = "BBHLHHHHQ8QQQH3H"
    HWTS_LOG_FMT = 'BBHHHQ12I'
    AIC_PMU_FMT = 'BBHHHII10Q8I'

    # stars
    ACSQ_TASK_FMT = "4HQHH11L"
    STARS_PCIE_FMT = "HHLQHBB3LQQ4L"
    ACC_PMU_FMT = "4HQ2H3L4Q"
    SOC_FMT = "HHLQ4LHH7L"
    FFTS_PMU_FMT = "4HQ4HQ12Q"
    FFTS_PLUS_PMU_FMT = "4HQ4HQ12Q"
    FFTS_LOG_FMT = "HHHHQHHBBH10L"
    FFTS_PLUS_LOG_FMT = "4HQ4H10L"
    LPS_FMT = "HHLQ12LHHLQ12LHHLQ12L"
    LPE_FMT = "HHLQQLL8L"
    STARS_FMT = "HHLQ12L"
    CHIP_TRANS_FMT = "2HLQ2H3L2Q4L"

    # other fmt
    STARS_HEADER_FMT = "=HH"
    RUNTIME_HEADER_FMT = "=BBH"
    STEP_HEADER_FMT = "=BB"
    HELPER_HEADER_FMT = "=HH"
    RUNTIME_RESERVED_FMT = "L"
    RUNTIME_API_FMT = "2HI3Q64s3I20HH106B"
    RUNTIME_API_FMT_SIZE = 256
    AI_CPU_FMT = 'HHHHQQQQQQQIIQQQQIIIHBBQ'
    AI_CPU_FMT_SIZE = 128
    TASK_NUM_OFFSET = struct.calcsize(BYTE_ORDER_CHAR + API_CALL_FMT[:-1])

    MSPROFTX_FMT = 'HHLLLLLQQQL128sL9Q'
    MSPROFTX_FMT_SIZE = struct.calcsize(BYTE_ORDER_CHAR + MSPROFTX_FMT)

    # ge
    GE_TASK_SIZE = 256
    GE_TENSOR_FMT = "HHIQIHHI55IQ"
    GE_TENSOR_SIZE = 256
    GE_STEP_FMT = "HHIIHHQQIB27B"
    GE_STEP_SIZE = 64
    GE_SESSION_FMT = "HHIIIQH6B"
    GE_SESSION_SIZE = 32

    # ge fusion
    GE_FUSION_PRE_FMT = "HHI8B"
    GE_FUSION_PRE_SIZE = 16
    GE_MODEL_LOAD_SIZE = 256
    GE_MODEL_TIME_SIZE = 256
    GE_FUSION_OP_SIZE = 256
    GE_HOST_FMT = 'HHL4Q24B'
    GE_HOST_FMT_SIZE = struct.calcsize(BYTE_ORDER_CHAR + GE_HOST_FMT)

    # helper
    HELPER_MODEL_WITH_Q_FMT = "HHIQQIHHQ24B"
    HELPER_MODEL_WITH_Q_FMT_SIZE = struct.calcsize(BYTE_ORDER_CHAR + HELPER_MODEL_WITH_Q_FMT)


    @staticmethod
    def class_name() -> str:
        """
        class name
        """
        return "StructFmt"

    @staticmethod
    def file_name() -> str:
        """
        file name
        """
        return "data_struct_size_constant"

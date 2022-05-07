# !/usr/bin/python
# coding=utf-8
"""
Function:
This file mainly decode binary data.
Copyright Information:
Huawei Technologies Co., Ltd. All Rights Reserved © 2020
"""

import struct

from msparser.data_struct_size_constant import StructFmt


class StructDecoder:
    """
    class for decode binary data
    """

    STRUCT_FMT_DICT = {
        "TimeLineData": StructFmt.TIME_LINE_FMT,
        "AiCoreTaskInfo": StructFmt.EVENT_COUNT_FMT,
        "StepTrace": StructFmt.STEP_TRACE_FMT,
        "TsMemcpy": StructFmt.TS_MEMCPY_FMT,
        "AiCpuData": StructFmt.AI_CPU_FMT,
        "AcsqTask": StructFmt.ACSQ_TASK_FMT,
        "AccPmuDecoder": StructFmt.ACC_PMU_FMT,
        "InterSoc": StructFmt.SOC_FMT,
        "StarsDecoder": StructFmt.STARS_FMT,
        "LpeDecoder": StructFmt.LPE_FMT,
        "LpsDecoder": StructFmt.LPS_FMT,
        "FftsPmuBean": StructFmt.FFTS_PMU_FMT,
        "FftsPlusPmuBean": StructFmt.FFTS_PLUS_PMU_FMT,
        "FftsLogDecoder": StructFmt.FFTS_LOG_FMT,
        "FftsPlusLogDecoder": StructFmt.FFTS_PLUS_LOG_FMT,
        "AicoreSample": StructFmt.AICORE_SAMPLE_FMT,
        "AivPmuBean": StructFmt.AIV_FMT,
        "TscpuDecoder": StructFmt.TSCPU_FMT,
        "MdcTscpuDecoder": StructFmt.MDC_TSCPU_FMT,
        "HwtsLogBean": StructFmt.HWTS_LOG_FMT,
        "AicPmuBean": StructFmt.AIC_PMU_FMT,
        "RunTimeApiBean": StructFmt.RUNTIME_API_FMT,
        "RtsDataBean": StructFmt.RTS_TRACK_FMT,
        "MsprofTxDecoder": StructFmt.MSPROFTX_FMT,
        "GeStepBean": StructFmt.GE_STEP_FMT,
        "GeSessionInfoBean": StructFmt.GE_SESSION_FMT,
        "GeTensorBean": StructFmt.GE_TENSOR_FMT,
        "GeHostBean": StructFmt.GE_HOST_FMT,
        "ModelWithQBean": StructFmt.HELPER_MODEL_WITH_Q_FMT,
        "StepTraceReader": StructFmt.STEP_TRACE_FMT,
        "StarsChipTransBean": StructFmt.CHIP_TRANS_FMT,
        "LowPowerBean": StructFmt.LOWPOWER_FMT,
        "Monitor0Bean": StructFmt.MONITOR0_FMT,
        "Monitor1Bean": StructFmt.MONITOR1_FMT,
        "TaskTypeBean": StructFmt.TS_TASK_TYPE_FMT
    }

    @classmethod
    def decode(cls: any, binary_data: bytes, additional_fmt: str = "") -> any:
        """
        decode binary dato to class
        :param binary_data:
        :param additional_fmt:
        :return:
        """
        fmt = StructFmt.BYTE_ORDER_CHAR + cls.get_fmt() + additional_fmt
        return cls(struct.unpack_from(fmt, binary_data))

    @classmethod
    def get_fmt(cls: any) -> str:
        """
        get fmt
        """
        return cls.STRUCT_FMT_DICT.get(str(cls.__name__))

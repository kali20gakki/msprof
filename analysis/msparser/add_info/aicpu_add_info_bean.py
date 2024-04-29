#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
import struct

from msparser.add_info.add_info_bean import AddInfoBean
from msparser.data_struct_size_constant import StructFmt
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.utils import Utils


class AicpuNodeBean:
    """
    aicpu data info bean
    """

    def __init__(self: any, *args) -> None:
        data = args[0]
        self._stream_id = Utils.get_stream_id(data[6])
        self._task_id = str(data[7])
        self._ai_cpu_task_start = data[10]
        self._compute_time = (data[12] - data[11]) / NumberConstant.MILLI_SECOND  # ms
        self._mem_copy_time = (data[13] - data[12]) / NumberConstant.MILLI_SECOND
        self._ai_cpu_task_end = data[15]
        self._dispatch_time = data[23] / NumberConstant.MILLI_SECOND
        self._total_time = data[24] / NumberConstant.MILLI_SECOND

    @property
    def stream_id(self: any) -> any:
        """
        stream id for ai cpu
        :return: stream id for ai cpu
        """
        return self._stream_id

    @property
    def task_id(self: any) -> any:
        """
        task id for ai cpu
        :return: task id for ai cpu
        """
        return self._task_id
    
    @property
    def compute_time(self: any) -> float:
        """
        ai cpu compute time
        :return: ai cpu compute time
        """
        return self._compute_time

    @property
    def memory_copy_time(self: any) -> float:
        """
        ai cpu memory copy time
        :return: ai cpu memory copy time
        """
        return self._mem_copy_time

    @property
    def ai_cpu_task_start_time(self: any) -> any:
        """
        ai cpu task start time
        :return: ai cpu task start time
        """
        if self._ai_cpu_task_start != 0:
            return InfoConfReader().time_from_syscnt(self._ai_cpu_task_start, NumberConstant.MILLI_SECOND)
        return 0

    @property
    def ai_cpu_task_start_syscnt(self: any) -> any:
        """
        ai cpu task start time
        :return: ai cpu task start time
        """
        return self._ai_cpu_task_start

    @property
    def ai_cpu_task_end_time(self: any) -> any:
        """
        ai cpu task end time
        :return: ai cpu task end time
        """
        if self._ai_cpu_task_end != 0:
            return InfoConfReader().time_from_syscnt(self._ai_cpu_task_end, NumberConstant.MILLI_SECOND)
        return 0

    @property
    def ai_cpu_task_end_syscnt(self: any) -> any:
        """
        ai cpu task end time
        :return: ai cpu task end time
        """
        return self._ai_cpu_task_end

    @property
    def ai_cpu_task_time(self: any) -> float:
        """
        ai cpu task time
        :return: ai cpu task time
        """
        return self.ai_cpu_task_end_time - self.ai_cpu_task_start_time

    @property
    def dispatch_time(self: any) -> float:
        """
        ai cpu dispatch time
        :return: ai cpu dispatch time
        """
        return self._dispatch_time

    @property
    def total_time(self: any) -> float:
        """
        ai cpu total time
        :return: ai cpu total time
        """
        return self._total_time


class AicpuDPBean:
    def __init__(self: any, *args) -> None:
        data = args[0]
        self._action = data[6].partition(b'\x00')[0].decode('utf-8', 'ignore')
        self._source = data[7].partition(b'\x00')[0].decode('utf-8', 'ignore')
        self._buffer_size = data[9]

    @property
    def action(self: any) -> str:
        return self._action

    @property
    def source(self: any) -> str:
        return self._source

    @property
    def buffer_size(self: any) -> int:
        return self._buffer_size


class AicpuModelBean:
    def __init__(self: any, *args) -> None:
        data = args[0]
        self._index_id = data[6]
        self._model_id = data[7]
        self._tag_id = data[8]
        self._event_id = data[10]

    @property
    def index_id(self: any) -> int:
        return self._index_id

    @property
    def model_id(self: any) -> int:
        return self._model_id

    @property
    def tag_id(self: any) -> int:
        return self._tag_id

    @property
    def event_id(self: any) -> int:
        return self._event_id


class AicpuMiBean:
    GET_NEXT_DEQUEUE_WAIT = 1
    NODE_NAME = {
        GET_NEXT_DEQUEUE_WAIT: "GetNext_dequeue_wait",
    }

    def __init__(self: any, *args) -> None:
        data = args[0]
        self._node_tag = data[6]
        self._queue_size = data[8]
        self._start_time = data[9]  # us
        self._end_time = data[10]

    @property
    def node_name(self: any) -> str:
        return self.NODE_NAME.get(self._node_tag, "")

    @property
    def queue_size(self: any) -> int:
        return self._queue_size

    @property
    def start_time(self: any) -> int:
        return self._start_time

    @property
    def end_time(self: any) -> int:
        return self._end_time

    @property
    def duration(self: any) -> int:
        return self._end_time - self._start_time


class AicpuAddInfoBean(AddInfoBean):
    """
    aicpu data info bean
    """
    AICPU_NODE = 0
    AICPU_DP = 1
    AICPU_MODEL = 2  # helper: MODEL_WITH_Q
    AICPU_MI = 3  # MindSpore

    STRUCT_FMT = {
        AICPU_NODE: StructFmt.AI_CPU_NODE_ADD_FMT,
        AICPU_DP: StructFmt.AI_CPU_DP_ADD_FMT,
        AICPU_MODEL: StructFmt.AI_CPU_MODEL_ADD_FMT,
        AICPU_MI: StructFmt.AI_CPU_MI_ADD_FMT,
    }

    AICPU_BEAN = {
        AICPU_NODE: AicpuNodeBean,
        AICPU_DP: AicpuDPBean,
        AICPU_MODEL: AicpuModelBean,
        AICPU_MI: AicpuMiBean,
    }

    def __init__(self: any, *args) -> None:
        super().__init__(*args)
        data = args[0]
        self._aicpu_data = None
        aicpu_bean = self.AICPU_BEAN.get(self._struct_type, None)
        if aicpu_bean:
            self._aicpu_data = aicpu_bean(data)

    @property
    def data(self: any) -> any:
        """
        ai cpu data
        :return: ai cpu data
        """
        return self._aicpu_data

    @classmethod
    def decode(cls: any, binary_data: bytes, additional_fmt: str = "") -> any:
        """
        decode binary data to class
        :param binary_data:
        :param additional_fmt:
        :return:
        """
        struct_type = struct.unpack("=I", binary_data[4:8])[0]
        fmt = StructFmt.BYTE_ORDER_CHAR + cls.STRUCT_FMT.get(struct_type, "") + additional_fmt
        return cls(struct.unpack_from(fmt, binary_data))

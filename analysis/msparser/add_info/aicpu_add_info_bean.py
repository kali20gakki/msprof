#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
import struct

from msparser.add_info.add_info_bean import AddInfoBean
from msparser.data_struct_size_constant import StructFmt
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.utils import Utils
from profiling_bean.stars.stars_common import StarsCommon


class AicpuNodeBean:
    """
    aicpu data info bean
    """

    def __init__(self: any, *args) -> None:
        data = args[0]
        self._stream_id = Utils.get_stream_id(data[6])
        self._task_id = StarsCommon.set_task_id(data[6], data[7])
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


class KfcHcclInfoBean:
    def __init__(self: any, *args) -> None:
        data = args[0]
        self._item_id = data[6]
        self._ccl_tag = data[7]
        self._group_name = data[8]
        self._local_rank = data[9]
        self._remote_rank = data[10]
        self._rank_size = data[11]
        self._work_flow_mode = data[12]
        self._plane_id = data[13]
        self._context_id = data[14]
        self._notify_id = data[15]
        self._stage = data[16]
        self._role = data[17]
        self._duration_estimated = data[18]
        self._src_addr = data[19]
        self._dst_addr = data[20]
        self._data_size = data[21]
        self._op_type = data[22]
        self._data_type = data[23]
        self._link_type = data[24]
        self._transport_type = data[25]
        self._rdma_type = data[26]
        self._task_id = StarsCommon.set_task_id(data[28], data[27])
        self._stream_id = Utils.get_stream_id(data[28])

    @property
    def item_id(self: any) -> str:
        return str(self._item_id)

    @property
    def ccl_tag(self: any) -> str:
        return str(self._ccl_tag)

    @property
    def group_name(self: any) -> str:
        return str(self._group_name)

    @property
    def local_rank(self: any) -> int:
        return self._local_rank

    @property
    def remote_rank(self: any) -> int:
        return self._remote_rank

    @property
    def rank_size(self: any) -> int:
        return self._rank_size

    @property
    def work_flow_mode(self: any) -> str:
        return str(self._work_flow_mode)

    @property
    def plane_id(self: any) -> int:
        return self._plane_id

    @property
    def context_id(self: any) -> int:
        return self._context_id

    @property
    def notify_id(self: any) -> str:
        return str(self._notify_id)

    @property
    def stage(self: any) -> str:
        return str(self._stage)

    @property
    def role(self: any) -> str:
        return str(self._role)

    @property
    def duration_estimated(self: any) -> int:
        return self._duration_estimated

    @property
    def src_addr(self: any) -> str:
        return str(self._src_addr)

    @property
    def dst_addr(self: any) -> str:
        return str(self._dst_addr)

    @property
    def data_size(self: any) -> int:
        return self._data_size

    @property
    def op_type(self: any) -> str:
        return str(self._op_type)

    @property
    def data_type(self: any) -> str:
        return str(self._data_type)

    @property
    def link_type(self: any) -> str:
        return str(self._link_type)

    @property
    def transport_type(self: any) -> str:
        return str(self._transport_type)

    @property
    def rdma_type(self: any) -> str:
        return str(self._rdma_type)

    @property
    def task_id(self: any) -> int:
        return self._task_id

    @property
    def stream_id(self: any) -> int:
        return self._stream_id


class KfcCommTurnBean:
    def __init__(self: any, *args) -> None:
        data = args[0]
        self._wait_notify_start_time = data[6]
        self._kfc_alg_exe_start_time = data[7]
        self._send_task_start_time = data[8]
        self._wait_active_start_time = data[9]
        self._active_start_time = data[10]
        self._wait_exe_end_start_time = data[11]
        self._rtsq_exe_end_time = data[12]
        self._data_len = data[13]
        self._device_id = data[14]
        self._stream_id = Utils.get_stream_id(data[15])
        self._task_id = StarsCommon.set_task_id(data[15], data[16])
        self._version = data[17]
        self._comm_turn = data[18]
        self._current_turn = data[19]

    @property
    def device_id(self: any) -> int:
        return self._device_id

    @property
    def stream_id(self: any) -> int:
        return self._stream_id

    @property
    def task_id(self: any) -> int:
        return self._task_id

    @property
    def version(self: any) -> int:
        return self._version

    @property
    def current_turn(self: any) -> int:
        return self._current_turn

    @property
    def comm_turn(self: any) -> int:
        return self._comm_turn

    @property
    def wait_notify_start_time(self: any) -> int:
        return self._wait_notify_start_time

    @property
    def kfc_alg_exe_start_time(self: any) -> int:
        return self._kfc_alg_exe_start_time

    @property
    def send_task_start_time(self: any) -> int:
        return self._send_task_start_time

    @property
    def wait_active_start_time(self: any) -> int:
        return self._wait_active_start_time

    @property
    def active_start_time(self: any) -> int:
        return self._active_start_time

    @property
    def wait_exe_end_start_time(self: any) -> int:
        return self._wait_exe_end_start_time

    @property
    def rtsq_exe_end_time(self: any) -> int:
        return self._rtsq_exe_end_time


class KfcComputeTurnBean:
    def __init__(self: any, *args) -> None:
        data = args[0]
        self._wait_compute_start_time = data[6]
        self._compute_start_time = data[7]
        self._compute_exe_end_time = data[8]
        self._data_len = data[9]
        self._device_id = data[10]
        self._stream_id = Utils.get_stream_id(data[11])
        self._task_id = StarsCommon.set_task_id(data[11], data[12])
        self._version = data[13]
        self._compute_turn = data[14]
        self._current_turn = data[15]

    @property
    def device_id(self: any) -> int:
        return self._device_id

    @property
    def stream_id(self: any) -> int:
        return self._stream_id

    @property
    def task_id(self: any) -> int:
        return self._task_id

    @property
    def version(self: any) -> int:
        return self._version

    @property
    def current_turn(self: any) -> int:
        return self._current_turn

    @property
    def compute_turn(self: any) -> int:
        return self._compute_turn

    @property
    def wait_compute_start_time(self: any) -> int:
        return self._wait_compute_start_time

    @property
    def compute_start_time(self: any) -> int:
        return self._compute_start_time

    @property
    def compute_exe_end_time(self: any) -> int:
        return self._compute_exe_end_time


class AicpuAddInfoBean(AddInfoBean):
    """
    aicpu data info bean
    """
    AICPU_NODE = 0
    AICPU_DP = 1
    AICPU_MODEL = 2  # helper: MODEL_WITH_Q
    AICPU_MI = 3  # MindSpore
    KFC_COMM_TURN = 4
    KFC_COMPUTE_TURN = 5
    KFC_HCCL_INFO = 6

    STRUCT_FMT = {
        AICPU_NODE: StructFmt.AI_CPU_NODE_ADD_FMT,
        AICPU_DP: StructFmt.AI_CPU_DP_ADD_FMT,
        AICPU_MODEL: StructFmt.AI_CPU_MODEL_ADD_FMT,
        AICPU_MI: StructFmt.AI_CPU_MI_ADD_FMT,
        KFC_COMM_TURN: StructFmt.KFC_COMM_TURN_FMT,
        KFC_COMPUTE_TURN: StructFmt.KFC_COMPUTE_TURN_FMT,
        KFC_HCCL_INFO: StructFmt.KFC_HCCL_INFO_FMT,
    }

    AICPU_BEAN = {
        AICPU_NODE: AicpuNodeBean,
        AICPU_DP: AicpuDPBean,
        AICPU_MODEL: AicpuModelBean,
        AICPU_MI: AicpuMiBean,
        KFC_COMM_TURN: KfcCommTurnBean,
        KFC_COMPUTE_TURN: KfcComputeTurnBean,
        KFC_HCCL_INFO: KfcHcclInfoBean,
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

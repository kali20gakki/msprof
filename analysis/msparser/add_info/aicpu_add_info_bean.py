#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

from msparser.add_info.add_info_bean import AddInfoBean
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.utils import Utils


class AicpuAddInfoBean(AddInfoBean):
    """
    aicpu data info bean
    """

    def __init__(self: any, *args) -> None:
        super().__init__(*args)
        data = args[0]
        self._stream_id = Utils.get_stream_id(data[6])
        self._task_id = str(data[7])
        self._ai_cpu_task_start = data[9]
        self._compute_time = (data[11] - data[10]) / NumberConstant.MILLI_SECOND
        self._mem_copy_time = (data[12] - data[11]) / NumberConstant.MILLI_SECOND
        self._ai_cpu_task_end = data[14]
        self._ai_cpu_task_time = data[14] - data[9]
        self._dispatch_time = data[22] / NumberConstant.MILLI_SECOND
        self._total_time = data[23] / NumberConstant.MILLI_SECOND

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

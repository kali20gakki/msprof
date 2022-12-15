#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.

from common_func.utils import Utils
from profiling_bean.struct_info.struct_decoder import StructDecoder


class FftsPlusPmuBean(StructDecoder):
    """
    class used to decode ffts pmu data
    """

    def __init__(self: any, *args: any) -> None:
        filed = args[0]
        self.func_type = Utils.get_func_type(filed[0])
        self._stream_id = Utils.get_stream_id(filed[2])
        self._task_id = filed[3]
        self._subtask_id = filed[6]
        self._ffts_type = filed[7] >> 13
        self._subtask_type = filed[5] & int(b'11111111')
        self._total_cycle = filed[12]
        self._pmu_list = filed[14:22]
        self._time_list = filed[22:]

    @property
    def stream_id(self: any) -> int:
        """
        get stream_id
        :return: stream_id
        """
        return self._stream_id

    @property
    def task_id(self: any) -> int:
        """
        get task_id
        :return: task_id
        """
        return self._task_id

    @property
    def subtask_id(self: any) -> int:
        """
        get subtask_id
        :return: subtask_id
        """
        return self._subtask_id

    @property
    def ffts_type(self: any) -> int:
        """
        get task_type
        :return: task_type
        """
        return self._ffts_type

    @property
    def subtask_type(self: any) -> int:
        """
        get task_type
        :return: task_type
        """
        return self._subtask_type

    @property
    def total_cycle(self: any) -> int:
        """
        get total_cycle
        :return: total_cycle
        """
        return self._total_cycle

    @property
    def pmu_list(self: any) -> list:
        """
        get pmu_list
        :return: pmu_list
        """
        return self._pmu_list

    @property
    def time_list(self: any) -> list:
        """
        get time_list
        :return: time_list
        """
        return self._time_list

    def is_aic_data(self):
        """
        get if aic data
        """
        return self.is_ffts_aic() or self.is_ffts_mix_aic_data() or self.is_tradition_aic()

    def is_ffts_aic(self):
        """
        get if ffts aic data
        """
        return bool(self._ffts_type == 4 and self.subtask_type == 0)

    def is_ffts_mix_aic_data(self):
        """
        get if ffts mix aic
        """
        return bool(self._ffts_type == 4 and self.subtask_type == 6) or bool(self._ffts_type == 5)

    def is_tradition_aic(self):
        """
        get if tradition aic
        """
        return bool(self._ffts_type == 0)


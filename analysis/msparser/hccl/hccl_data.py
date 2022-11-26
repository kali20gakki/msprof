#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

from common_func.constant import Constant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant


class HCCLData:
    """
    HCCL data parser.
    """

    def __init__(self: any, hccl_data: dict) -> None:
        self._hccl_data = hccl_data
        self._hccl_name = Constant.NA
        self._plane_id = -1
        self._timestamp = -1
        self._duration = -1
        self._bandwidth = Constant.NA
        self._stage = -1
        self._step = -1
        self._stream_id = -1
        self._task_id = -1
        self._task_type = Constant.NA
        self._src_rank = -1
        self._dst_rank = -1
        self._notify_id = -1
        self._transport_type = Constant.NA
        self._size = -1

    @property
    def hccl_name(self: any) -> str:
        """
        hccl name
        :return: hccl name
        """
        return self._hccl_data.get('name', self._hccl_name)

    @property
    def plane_id(self: any) -> str:
        """
        hccl plane id
        :return: hccl plane id
        """
        return self._hccl_data.get('tid', self._plane_id)

    @property
    def timestamp(self: any) -> float:
        """
        hccl timestamp
        :return: hccl timestamp
        """
        return InfoConfReader().time_from_syscnt(
            self._hccl_data.get('ts', self._timestamp) * InfoConfReader().get_freq(StrConstant.HWTS) / 1000000.0,
            NumberConstant.MICRO_SECOND)

    @property
    def duration(self: any) -> int:
        """
        hccl duration
        :return: hccl duration
        """
        return self._hccl_data.get('dur', self._duration)

    @property
    def bandwidth(self: any) -> int:
        """
        hccl bandwidth
        :return: hccl bandwidth
        """
        return self._hccl_data.get("args", {}).get('bandwidth', self._bandwidth)

    @property
    def stream_id(self: any) -> int:
        """
        hccl stream_id
        :return: hccl stream_id
        """
        return self._hccl_data.get("args", {}).get('stream id', self._stream_id)

    @property
    def task_id(self: any) -> int:
        """
        hccl task_id
        :return: hccl task_id
        """
        return self._hccl_data.get("args", {}).get('task id', self._task_id)

    @property
    def task_type(self: any) -> int:
        """
        hccl task type
        :return: hccl task_type
        """
        return self._hccl_data.get("args", {}).get('task type', self._task_type)

    @property
    def notify_id(self: any) -> int:
        """
        hccl notify id
        :return: hccl task_type
        """
        return self._hccl_data.get("args", {}).get('notify id', self._notify_id)

    @property
    def stage(self: any) -> int:
        """
        hccl stage
        :return: hccl task_type
        """
        return self._hccl_data.get("args", {}).get('stage', self._stage)

    @property
    def step(self: any) -> int:
        """
        hccl step
        :return: hccl task_type
        """
        return self._hccl_data.get("args", {}).get('step', self._step)

    @property
    def dst_rank(self: any) -> int:
        """
        hccl dst rank
        :return: hccl task_type
        """
        return self._hccl_data.get("args", {}).get('dst rank', self._dst_rank)

    @property
    def src_rank(self: any) -> int:
        """
        hccl src rank
        :return: hccl dst rank
        """
        return self._hccl_data.get("args", {}).get('src rank', self._src_rank)

    @property
    def transport_type(self: any) -> int:
        """
        hccl transport type
        :return: hccl transport type
        """
        return self._hccl_data.get("args", {}).get('transport type', self._transport_type)

    @property
    def size(self: any) -> int:
        """
        hccl size
        :return: hccl size
        """
        return self._hccl_data.get("args", {}).get('size', self._size)

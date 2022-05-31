#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2022. All rights reserved.
"""
from common_func.constant import Constant


class HcclDto:
    """
    Dto for hccl data
    """

    def __init__(self: any) -> None:
        self._hccl_name = Constant.NA
        self._plane_id = 0
        self._timestamp = 0
        self._duration = 0
        self._bandwidth = Constant.NULL
        self._stage = 0
        self._step = 0
        self._stream_id = 0
        self._task_id = 0
        self._task_type = Constant.NA
        self._src_rank = 0
        self._dst_rank = 0
        self._notify_id = 0
        self._transport_type = Constant.NA
        self._size = 0

    @property
    def hccl_name(self: any) -> any:
        return self._hccl_name

    @hccl_name.setter
    def hccl_name(self: any, value: any) -> None:
        self._hccl_name = value

    @property
    def plane_id(self: any) -> any:
        return self._plane_id

    @plane_id.setter
    def plane_id(self: any, value: any) -> None:
        self._plane_id = value

    @property
    def timestamp(self: any) -> any:
        return self._timestamp

    @timestamp.setter
    def timestamp(self: any, value: any) -> None:
        self._timestamp = value

    @property
    def duration(self: any) -> any:
        return self._duration

    @duration.setter
    def duration(self: any, value: any) -> None:
        self._duration = value

    @property
    def bandwidth(self: any) -> any:
        return self._bandwidth

    @bandwidth.setter
    def bandwidth(self: any, value: any) -> None:
        self._bandwidth = value

    @property
    def stream_id(self: any) -> any:
        return self._stream_id

    @stream_id.setter
    def stream_id(self: any, value: any) -> None:
        self._stream_id = value

    @property
    def task_id(self: any) -> any:
        return self._task_id

    @task_id.setter
    def task_id(self: any, value: any) -> None:
        self._task_id = value

    @property
    def task_type(self: any) -> any:
        return self._task_type

    @task_type.setter
    def task_type(self: any, value: any) -> None:
        self._task_type = value

    @property
    def notify_id(self: any) -> any:
        return self._notify_id

    @notify_id.setter
    def notify_id(self: any, value: any) -> None:
        self._notify_id = value

    @property
    def stage(self: any) -> any:
        return self._stage

    @stage.setter
    def stage(self: any, value: any) -> None:
        self._stage = value

    @property
    def step(self: any) -> any:
        return self._step

    @step.setter
    def step(self: any, value: any) -> None:
        self._step = value

    @property
    def dst_rank(self: any) -> any:
        return self._dst_rank

    @dst_rank.setter
    def dst_rank(self: any, value: any) -> None:
        self._dst_rank = value

    @property
    def src_rank(self: any) -> any:
        return self._src_rank

    @src_rank.setter
    def src_rank(self: any, value: any) -> None:
        self._src_rank = value

    @property
    def transport_type(self: any) -> any:
        return self._transport_type

    @transport_type.setter
    def transport_type(self: any, value: any) -> None:
        self._transport_type = value

    @property
    def size(self: any) -> any:
        return self._size

    @size.setter
    def size(self: any, value: any) -> None:
        self._size = value

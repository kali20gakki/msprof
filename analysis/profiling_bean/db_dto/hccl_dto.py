#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
import ast

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
        self._args = {}
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
    def args(self: any) -> any:
        return self._args

    @args.setter
    def args(self: any, value: any) -> None:
        self._args = ast.literal_eval(value)

    @property
    def bandwidth(self: any) -> any:
        return self._args.get('bandwidth', self._bandwidth)

    @property
    def stream_id(self: any) -> any:
        return self._args.get('stream id', self._stream_id)

    @property
    def task_id(self: any) -> any:
        return self._args.get('task id', self._task_id)

    @property
    def task_type(self: any) -> any:
        return self._args.get('task type', self._task_type)

    @property
    def notify_id(self: any) -> any:
        return self._args.get('notify id', self._notify_id)

    @property
    def stage(self: any) -> any:
        return self._args.get('stage', self._stage)

    @property
    def step(self: any) -> any:
        return self._args.get('step', self._step)

    @property
    def dst_rank(self: any) -> any:
        return self._args.get('dst rank', self._dst_rank)

    @property
    def src_rank(self: any) -> any:
        return self._args.get('src rank', self._src_rank)

    @property
    def transport_type(self: any) -> any:
        return self._args.get('transport type', self._transport_type)

    @property
    def size(self: any) -> any:
        return self._args.get('size', self._size)

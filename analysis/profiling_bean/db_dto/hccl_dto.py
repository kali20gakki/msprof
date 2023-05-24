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
        self._index_id = Constant.DEFAULT_INVALID_VALUE
        self._model_id = Constant.DEFAULT_INVALID_VALUE
        self._is_dynamic = Constant.DEFAULT_INVALID_VALUE
        self._op_name = Constant.NA
        self._iteration = Constant.DEFAULT_VALUE
        self._hccl_name = Constant.NA
        self._first_timestamp = Constant.DEFAULT_VALUE
        self._plane_id = Constant.DEFAULT_VALUE
        self._timestamp = Constant.DEFAULT_VALUE
        self._duration = Constant.DEFAULT_VALUE
        self._args = {}
        self._bandwidth = Constant.NA
        self._stage = Constant.DEFAULT_INVALID_VALUE
        self._step = Constant.DEFAULT_INVALID_VALUE
        self._stream_id = Constant.DEFAULT_INVALID_VALUE
        self._task_id = Constant.DEFAULT_INVALID_VALUE
        self._task_type = Constant.NA
        self._op_type = Constant.NA
        self._src_rank = Constant.DEFAULT_INVALID_VALUE
        self._dst_rank = Constant.DEFAULT_INVALID_VALUE
        self._notify_id = Constant.DEFAULT_INVALID_VALUE
        self._transport_type = Constant.NA
        self._size = Constant.DEFAULT_INVALID_VALUE

    @property
    def op_name(self: any) -> str:
        return self._op_name

    @property
    def iteration(self: any) -> int:
        return self._iteration

    @property
    def hccl_name(self: any) -> any:
        return self._args.get('task type', self._hccl_name)

    @property
    def first_timestamp(self: any) -> float:
        return self._first_timestamp

    @property
    def plane_id(self: any) -> any:
        return self._plane_id

    @property
    def timestamp(self: any) -> any:
        return self._timestamp

    @property
    def duration(self: any) -> any:
        return self._duration

    @property
    def args(self: any) -> any:
        return self._args

    @property
    def bandwidth(self: any) -> any:
        if self._args.get('bandwidth', self._bandwidth) == Constant.NA:
            return self._args.get('bandwidth(Gbyte/s)', self._bandwidth)
        return self._args.get('bandwidth', self._bandwidth)

    @property
    def stream_id(self: any) -> any:
        return self._args.get('stream id', self._stream_id)

    @property
    def task_id(self: any) -> any:
        return self._args.get('task id', self._task_id)

    @property
    def task_type(self: any) -> any:
        return self._task_type

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
        if self._args.get('size', self._size) == Constant.DEFAULT_INVALID_VALUE:
            return self._args.get('size(Byte)', self._size)
        return self._args.get('size', self._size)

    @property
    def is_dynamic(self) -> int:
        return self._is_dynamic

    @property
    def model_id(self) -> int:
        return self._model_id

    @property
    def index_id(self) -> int:
        return self._index_id

    @property
    def op_type(self) -> int:
        return self._op_type

    @op_name.setter
    def op_name(self: any, value: str) -> None:
        self._op_name = value

    @iteration.setter
    def iteration(self: any, value: int) -> None:
        self._iteration = value

    @hccl_name.setter
    def hccl_name(self: any, value: any) -> None:
        self._hccl_name = value

    @first_timestamp.setter
    def first_timestamp(self: any, value: float) -> None:
        self._first_timestamp = value

    @plane_id.setter
    def plane_id(self: any, value: any) -> None:
        self._plane_id = value

    @timestamp.setter
    def timestamp(self: any, value: any) -> None:
        self._timestamp = value

    @duration.setter
    def duration(self: any, value: any) -> None:
        self._duration = value

    @args.setter
    def args(self: any, value: any) -> None:
        self._args = ast.literal_eval(value)

    @is_dynamic.setter
    def is_dynamic(self, value: str) -> None:
        self._is_dynamic = value

    @model_id.setter
    def model_id(self, value: str):
        self._model_id = value

    @index_id.setter
    def index_id(self, value: str):
        self._index_id = value

    @task_type.setter
    def task_type(self, value: str):
        self._task_type = value

    @op_type.setter
    def op_type(self, value: str):
        self._op_type = value

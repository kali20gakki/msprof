#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.

from common_func.constant import Constant


class TimeSectionDto:
    def __init__(self: any):
        self._op_name = None
        self._task_type = None
        self._duration_time = Constant.DEFAULT_INVALID_VALUE
        self._model_id = Constant.DEFAULT_INVALID_VALUE
        self._index_id = Constant.DEFAULT_INVALID_VALUE
        self._stream_id = Constant.DEFAULT_INVALID_VALUE
        self._task_id = Constant.DEFAULT_INVALID_VALUE
        self._start_time = Constant.DEFAULT_INVALID_VALUE
        self._end_time = Constant.DEFAULT_INVALID_VALUE
        self._overlap_time = Constant.DEFAULT_INVALID_VALUE

    def __lt__(self, other):
        return self.start_time < other.start_time

    @property
    def op_name(self: any) -> any:
        return self._op_name

    @property
    def task_type(self: any) -> any:
        return self._task_type

    @property
    def duration_time(self: any) -> any:
        return float(self._duration_time)

    @property
    def model_id(self: any) -> any:
        return float(self._model_id)

    @property
    def index_id(self: any) -> any:
        return float(self._index_id)

    @property
    def stream_id(self: any) -> any:
        return float(self._stream_id)

    @property
    def task_id(self: any) -> any:
        return float(self._task_id)

    @property
    def start_time(self: any) -> any:
        return float(self._start_time)

    @property
    def end_time(self: any) -> any:
        return float(self._end_time)

    @property
    def overlap_time(self: any) -> any:
        return float(self._overlap_time)

    @op_name.setter
    def op_name(self: any, value: any) -> None:
        self._op_name = value

    @task_type.setter
    def task_type(self: any, value: any) -> None:
        self._task_type = value

    @model_id.setter
    def model_id(self: any, value: any) -> None:
        self._model_id = value

    @duration_time.setter
    def duration_time(self: any, value: any) -> None:
        self._duration_time = value

    @index_id.setter
    def index_id(self: any, value: any) -> None:
        self._index_id = value

    @stream_id.setter
    def stream_id(self: any, value: any) -> None:
        self._stream_id = value

    @task_id.setter
    def task_id(self: any, value: any) -> None:
        self._task_id = value

    @start_time.setter
    def start_time(self: any, value: any) -> None:
        self._start_time = value

    @end_time.setter
    def end_time(self: any, value: any) -> None:
        self._end_time = value

    @overlap_time.setter
    def overlap_time(self: any, value: any) -> None:
        self._overlap_time = value


class CommunicationTimeSection(TimeSectionDto):
    def __init__(self):
        super().__init__()

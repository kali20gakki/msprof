#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.

from common_func.constant import Constant


class TimeSectionDto:
    def __init__(self: any):
        self._model_id = Constant.DEFAULT_INVALID_VALUE
        self._index_id = Constant.DEFAULT_INVALID_VALUE
        self._stream_id = Constant.DEFAULT_INVALID_VALUE
        self._task_id = Constant.DEFAULT_INVALID_VALUE
        self._start_time = Constant.DEFAULT_INVALID_VALUE
        self._end_time = Constant.DEFAULT_INVALID_VALUE
        self._overlap_time = Constant.DEFAULT_INVALID_VALUE

    @property
    def model_id(self: any) -> any:
        return float(self._model_id)

    @model_id.setter
    def model_id(self: any, value: any) -> None:
        self._model_id = value

    @property
    def index_id(self: any) -> any:
        return float(self._index_id)

    @index_id.setter
    def index_id(self: any, value: any) -> None:
        self._index_id = value

    @property
    def stream_id(self: any) -> any:
        return float(self._stream_id)

    @stream_id.setter
    def stream_id(self: any, value: any) -> None:
        self._stream_id = value

    @property
    def task_id(self: any) -> any:
        return float(self._task_id)

    @task_id.setter
    def task_id(self: any, value: any) -> None:
        self._task_id = value

    @property
    def start_time(self: any) -> any:
        return float(self._start_time)

    @start_time.setter
    def start_time(self: any, value: any) -> None:
        self._start_time = value

    @property
    def end_time(self: any) -> any:
        return float(self._end_time)

    @end_time.setter
    def end_time(self: any, value: any) -> None:
        self._end_time = value

    @property
    def overlap_time(self: any) -> any:
        return float(self._overlap_time)

    @overlap_time.setter
    def overlap_time(self: any, value: any) -> None:
        self._overlap_time = value

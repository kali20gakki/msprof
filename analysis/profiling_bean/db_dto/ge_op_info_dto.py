#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.


class GeOpInfoDto:
    """
    Dto for ge op info data
    """

    def __init__(self: any) -> None:
        self._model_id = None
        self._task_id = None
        self._stream_id = None
        self._op_name = None
        self._op_type = None
        self._task_type = None
        self._start_time = None
        self._duration_time = None

    @property
    def model_id(self: any) -> any:
        return self._model_id

    @property
    def task_id(self: any) -> any:
        return self._task_id

    @property
    def stream_id(self: any) -> any:
        return self._stream_id

    @property
    def op_name(self: any) -> any:
        return self._op_name

    @property
    def op_type(self: any) -> any:
        return self._op_type

    @property
    def task_type(self: any) -> any:
        return self._task_type

    @property
    def start_time(self: any) -> any:
        return self._start_time

    @property
    def duration_time(self: any) -> any:
        return self._duration_time

    @model_id.setter
    def model_id(self: any, value: any) -> None:
        self._model_id = value

    @task_id.setter
    def task_id(self: any, value: any) -> None:
        self._task_id = value

    @stream_id.setter
    def stream_id(self: any, value: any) -> None:
        self._stream_id = value

    @op_name.setter
    def op_name(self: any, value: any) -> None:
        self._op_name = value

    @op_type.setter
    def op_type(self: any, value: any) -> None:
        self._op_type = value

    @task_type.setter
    def task_type(self: any, value: any) -> None:
        self._task_type = value

    @start_time.setter
    def start_time(self: any, value: any) -> None:
        self._start_time = value

    @duration_time.setter
    def duration_time(self: any, value: any) -> None:
        self._duration_time = value

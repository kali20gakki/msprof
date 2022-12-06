#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.


class TaskTimeDto:
    """
    Dto for stars data or hwts data
    """

    def __init__(self: any) -> None:
        self._op_name = None
        self._stream_id = None
        self._task_id = None
        self._task_type = None
        self._task_time = None
        self._start_time = None
        self._end_time = None

    @property
    def op_name(self: any) -> any:
        return self._op_name

    @op_name.setter
    def op_name(self: any, value: any) -> None:
        self._op_name = value

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
    def task_time(self: any) -> any:
        return self._task_time

    @task_time.setter
    def task_time(self: any, value: any) -> None:
        self._task_time = value

    @property
    def start_time(self: any) -> any:
        return self._start_time

    @start_time.setter
    def start_time(self: any, value: any) -> None:
        self._start_time = value

    @property
    def end_time(self: any) -> any:
        return self._end_time

    @end_time.setter
    def end_time(self: any, value: any) -> None:
        self._end_time = value

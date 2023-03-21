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
        self._batch_id = None
        self._subtask_id = None
        self._task_type = None
        self._subtask_type = None
        self._task_time = None
        self._dur_time = None
        self._start_time = None
        self._end_time = None
        self._ffts_type = None
        self._thread_id = None

    @property
    def op_name(self: any) -> any:
        return self._op_name

    @property
    def stream_id(self: any) -> any:
        return self._stream_id

    @property
    def task_id(self: any) -> any:
        return self._task_id

    @property
    def batch_id(self: any) -> any:
        return self._batch_id

    @property
    def subtask_id(self: any) -> any:
        return self._subtask_id

    @property
    def thread_id(self: any) -> any:
        return self._thread_id

    @property
    def task_type(self: any) -> any:
        return self._task_type

    @property
    def subtask_type(self: any) -> any:
        return self._subtask_type

    @property
    def ffts_type(self: any) -> any:
        return self._ffts_type

    @property
    def task_time(self: any) -> any:
        return self._task_time

    @property
    def start_time(self: any) -> any:
        return self._start_time

    @property
    def dur_time(self: any) -> any:
        return self._dur_time

    @property
    def end_time(self: any) -> any:
        return self._end_time

    @op_name.setter
    def op_name(self: any, value: any) -> None:
        self._op_name = value

    @stream_id.setter
    def stream_id(self: any, value: any) -> None:
        self._stream_id = value

    @task_id.setter
    def task_id(self: any, value: any) -> None:
        self._task_id = value

    @batch_id.setter
    def batch_id(self: any, value: any) -> None:
        self._batch_id = value

    @subtask_id.setter
    def subtask_id(self: any, value: any) -> None:
        self._subtask_id = value

    @thread_id.setter
    def thread_id(self: any, value: any) -> None:
        self._thread_id = value

    @task_type.setter
    def task_type(self: any, value: any) -> None:
        self._task_type = value

    @subtask_type.setter
    def subtask_type(self: any, value: any) -> None:
        self._subtask_type = value

    @ffts_type.setter
    def ffts_type(self: any, value: any) -> None:
        self._ffts_type = value

    @task_time.setter
    def task_time(self: any, value: any) -> None:
        self._task_time = value

    @dur_time.setter
    def dur_time(self: any, value: any) -> None:
        self._dur_time = value

    @start_time.setter
    def start_time(self: any, value: any) -> None:
        self._start_time = value

    @end_time.setter
    def end_time(self: any, value: any) -> None:
        self._end_time = value

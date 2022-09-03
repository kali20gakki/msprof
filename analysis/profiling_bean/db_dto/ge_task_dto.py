#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.


class GeTaskDto:
    """
    Dto for acl data
    """

    def __init__(self: any) -> None:
        self._op_name = None
        self._stream_id = None
        self._task_id = None
        self._batch_id = None
        self._tasktype = None

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
    def batch_id(self: any) -> any:
        return self._batch_id

    @batch_id.setter
    def batch_id(self: any, value: any) -> None:
        self._batch_id = value

    @property
    def tasktype(self: any) -> any:
        return self._tasktype

    @tasktype.setter
    def tasktype(self: any, value: any) -> None:
        self._tasktype = value

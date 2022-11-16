#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.


class GeTaskDto:
    """
    Dto for acl data
    """

    def __init__(self: any) -> None:
        self._model_id = None
        self._op_name = None
        self._stream_id = None
        self._task_id = None
        self._block_dim = None
        self._op_state = None
        self._batch_id = None
        self._context_id = None
        self._task_type = None
        self._op_type = None
        self._index_id = None
        self._thread_id = None
        self._timestamp = None
        self._batch_id = None

    @property
    def model_id(self: any) -> any:
        return self._model_id

    @model_id.setter
    def model_id(self: any, value: any) -> None:
        self._model_id = value

    @property
    def context_id(self: any) -> any:
        return self._context_id

    @context_id.setter
    def context_id(self: any, value: any) -> None:
        self._context_id = value

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
    def block_dim(self: any) -> any:
        return self._block_dim

    @block_dim.setter
    def block_dim(self: any, value: any) -> None:
        self._block_dim = value

    @property
    def op_state(self: any) -> any:
        return self._op_state

    @op_state.setter
    def op_state(self: any, value: any) -> None:
        self._op_state = value

    @property
    def task_type(self: any) -> any:
        return self._task_type

    @task_type.setter
    def task_type(self: any, value: any) -> None:
        self._task_type = value

    @property
    def op_type(self: any) -> any:
        return self._op_type

    @op_type.setter
    def op_type(self: any, value: any) -> None:
        self._op_type = value

    @property
    def index_id(self: any) -> any:
        return self._index_id

    @index_id.setter
    def index_id(self: any, value: any) -> None:
        self._index_id = value

    @property
    def timestamp(self: any) -> any:
        return self._timestamp

    @timestamp.setter
    def timestamp(self: any, value: any) -> None:
        self._timestamp = value

    @property
    def thread_id(self: any) -> any:
        return self._thread_id

    @thread_id.setter
    def thread_id(self: any, value: any) -> None:
        self._thread_id = value

    @property
    def batch_id(self: any) -> any:
        return self._batch_id

    @batch_id.setter
    def batch_id(self: any, value: any) -> None:
        self._batch_id = value

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
        self._mix_block_dim = None
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

    @property
    def context_id(self: any) -> any:
        return self._context_id

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
    def block_dim(self: any) -> any:
        return self._block_dim

    @property
    def mix_block_dim(self: any) -> any:
        return self._mix_block_dim

    @property
    def op_state(self: any) -> any:
        return self._op_state

    @property
    def task_type(self: any) -> any:
        return self._task_type

    @property
    def op_type(self: any) -> any:
        return self._op_type

    @property
    def index_id(self: any) -> any:
        return self._index_id

    @property
    def timestamp(self: any) -> any:
        return self._timestamp

    @property
    def thread_id(self: any) -> any:
        return self._thread_id

    @property
    def batch_id(self: any) -> any:
        return self._batch_id

    @model_id.setter
    def model_id(self: any, value: any) -> None:
        self._model_id = value

    @context_id.setter
    def context_id(self: any, value: any) -> None:
        self._context_id = value

    @op_name.setter
    def op_name(self: any, value: any) -> None:
        self._op_name = value

    @stream_id.setter
    def stream_id(self: any, value: any) -> None:
        self._stream_id = value

    @task_id.setter
    def task_id(self: any, value: any) -> None:
        self._task_id = value

    @block_dim.setter
    def block_dim(self: any, value: any) -> None:
        self._block_dim = value

    @mix_block_dim.setter
    def mix_block_dim(self: any, value: any) -> None:
        self._mix_block_dim = value

    @op_state.setter
    def op_state(self: any, value: any) -> None:
        self._op_state = value

    @task_type.setter
    def task_type(self: any, value: any) -> None:
        self._task_type = value

    @op_type.setter
    def op_type(self: any, value: any) -> None:
        self._op_type = value

    @index_id.setter
    def index_id(self: any, value: any) -> None:
        self._index_id = value

    @timestamp.setter
    def timestamp(self: any, value: any) -> None:
        self._timestamp = value

    @thread_id.setter
    def thread_id(self: any, value: any) -> None:
        self._thread_id = value

    @batch_id.setter
    def batch_id(self: any, value: any) -> None:
        self._batch_id = value

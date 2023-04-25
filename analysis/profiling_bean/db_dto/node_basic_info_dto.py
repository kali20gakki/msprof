#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.


class NodeBasicInfoDto:
    def __init__(self: any) -> None:
        self._level = None
        self._type = None
        self._thread_id = None
        self._data_len = None
        self._timestamp = None
        self._node_id = None
        self._task_type = None
        self._op_type = None
        self._block_dim = None
        self._mix_block_dim = None
        self._op_flag = None

    @property
    def level(self: any) -> str:
        return str(self._level)

    @property
    def data_type(self: any) -> str:
        return str(self._type)

    @property
    def thread_id(self: any) -> int:
        return self._thread_id

    @property
    def data_len(self: any) -> int:
        return self._data_len

    @property
    def timestamp(self: any) -> int:
        return self._timestamp

    @property
    def node_id(self: any) -> int:
        return self._node_id

    @property
    def task_type(self: any) -> int:
        return self._task_type

    @property
    def op_type(self: any) -> int:
        return self._op_type

    @property
    def block_dim(self: any) -> int:
        return self._block_dim

    @property
    def op_flag(self: any) -> int:
        return self._op_flag

    @property
    def mix_block_dim(self):
        return self._mix_block_dim

    @level.setter
    def level(self: any, value: any) -> None:
        self._level = value

    @data_type.setter
    def data_type(self: any, value: any) -> None:
        self._type = value

    @thread_id.setter
    def thread_id(self: any, value: any) -> None:
        self._thread_id = value

    @data_len.setter
    def data_len(self: any, value: any) -> None:
        self._data_len = value

    @timestamp.setter
    def timestamp(self: any, value: any) -> None:
        self._timestamp = value

    @node_id.setter
    def node_id(self: any, value: any) -> None:
        self._node_id = value

    @task_type.setter
    def task_type(self: any, value: any) -> None:
        self._task_type = value

    @op_type.setter
    def op_type(self: any, value: any) -> None:
        self._op_type = value

    @block_dim.setter
    def block_dim(self: any, value: any) -> None:
        self._block_dim = value

    @op_flag.setter
    def op_flag(self: any, value: any) -> None:
        self._op_flag = value

    @mix_block_dim.setter
    def mix_block_dim(self, value):
        self._mix_block_dim = value

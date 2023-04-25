#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.


class CtxIdDto:
    def __init__(self: any) -> None:
        self._level = None
        self._type = None
        self._thread_id = None
        self._data_len = None
        self._timestamp = None
        self._node_id = None
        self._ctx_id_num = None
        self._ctx_id = None

    @property
    def level(self: any) -> str:
        return self._level

    @property
    def type(self: any) -> str:
        return str(self._type)

    @property
    def thread_id(self: any) -> int:
        return self._thread_id

    @property
    def data_len(self):
        return self._data_len

    @property
    def timestamp(self):
        return self._timestamp

    @property
    def node_id(self):
        return self._node_id

    @property
    def ctx_id_num(self):
        return self._ctx_id_num

    @property
    def ctx_id(self):
        return self._ctx_id

    @level.setter
    def level(self: any, value: any) -> None:
        self._level = value

    @type.setter
    def type(self: any, value: any) -> None:
        self._type = value

    @thread_id.setter
    def thread_id(self: any, value: any) -> None:
        self._thread_id = value

    @data_len.setter
    def data_len(self, value):
        self._data_len = value

    @timestamp.setter
    def timestamp(self, value):
        self._timestamp = value

    @node_id.setter
    def node_id(self, value):
        self._node_id = value

    @ctx_id_num.setter
    def ctx_id_num(self, value):
        self._ctx_id_num = value

    @ctx_id.setter
    def ctx_id(self, value):
        self._ctx_id = value

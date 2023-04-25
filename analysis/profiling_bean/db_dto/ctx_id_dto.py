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

    @level.setter
    def level(self: any, value: any) -> None:
        self._level = value

    @property
    def type(self: any) -> str:
        return str(self._type)

    @type.setter
    def type(self: any, value: any) -> None:
        self._type = value

    @property
    def thread_id(self: any) -> int:
        return self._thread_id

    @thread_id.setter
    def thread_id(self: any, value: any) -> None:
        self._thread_id = value

    @property
    def data_len(self):
        return self._data_len

    @data_len.setter
    def data_len(self, value):
        self._data_len = value

    @property
    def timestamp(self):
        return self._timestamp

    @timestamp.setter
    def timestamp(self, value):
        self._timestamp = value

    @property
    def node_id(self):
        return self._node_id

    @node_id.setter
    def node_id(self, value):
        self._node_id = value

    @property
    def ctx_id_num(self):
        return self._ctx_id_num

    @ctx_id_num.setter
    def ctx_id_num(self, value):
        self._ctx_id_num = value

    @property
    def ctx_id(self):
        return self._ctx_id

    @ctx_id.setter
    def ctx_id(self, value):
        self._ctx_id = value

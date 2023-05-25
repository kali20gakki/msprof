#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.


class CtxIdDto:
    INVALID_CONTEXT_ID = 4294967295

    def __init__(self: any) -> None:
        self._level = None
        self._struct_type = None
        self._thread_id = None
        self._data_len = None
        self._timestamp = None
        self._op_name = None
        self._ctx_id_num = None
        self._ctx_id = self.INVALID_CONTEXT_ID

    @property
    def level(self: any) -> str:
        return self._level

    @property
    def struct_type(self: any) -> str:
        return str(self._struct_type)

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
    def op_name(self):
        return self._op_name

    @property
    def ctx_id_num(self):
        return self._ctx_id_num

    @property
    def ctx_id(self):
        return self._ctx_id

    @level.setter
    def level(self: any, value: any) -> None:
        self._level = value

    @struct_type.setter
    def struct_type(self: any, value: any) -> None:
        self._struct_type = value

    @thread_id.setter
    def thread_id(self: any, value: any) -> None:
        self._thread_id = value

    @data_len.setter
    def data_len(self, value):
        self._data_len = value

    @timestamp.setter
    def timestamp(self, value):
        self._timestamp = value

    @op_name.setter
    def op_name(self, value):
        self._op_name = value

    @ctx_id_num.setter
    def ctx_id_num(self, value):
        self._ctx_id_num = value

    @ctx_id.setter
    def ctx_id(self, value):
        self._ctx_id = value

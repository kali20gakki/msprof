#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.


class FusionOpInfoDto:
    def __init__(self: any) -> None:
        self._level = None
        self._struct_type = None
        self._thread_id = None
        self._data_len = None
        self._timestamp = None
        self._op_name = None
        self._fusion_op_num = None
        self._memory_input = None
        self._memory_output = None
        self._memory_weight = None
        self._memory_workspace = None
        self._memory_total = None
        self._fusion_op_names = None

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
    def fusion_op_num(self):
        return self._fusion_op_num

    @property
    def memory_input(self):
        return self._memory_input

    @property
    def memory_output(self):
        return self._memory_output

    @property
    def memory_weight(self):
        return self._memory_weight

    @property
    def memory_workspace(self):
        return self._memory_workspace

    @property
    def memory_total(self):
        return self._memory_total

    @property
    def fusion_op_names(self):
        return self._fusion_op_names

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

    @fusion_op_num.setter
    def fusion_op_num(self, value):
        self._fusion_op_num = value

    @memory_input.setter
    def memory_input(self, value):
        self._memory_input = value

    @memory_output.setter
    def memory_output(self, value):
        self._memory_output = value

    @memory_weight.setter
    def memory_weight(self, value):
        self._memory_weight = value

    @memory_workspace.setter
    def memory_workspace(self, value):
        self._memory_workspace = value

    @memory_total.setter
    def memory_total(self, value):
        self._memory_total = value

    @fusion_op_names.setter
    def fusion_op_names(self, value):
        self._fusion_op_names = value

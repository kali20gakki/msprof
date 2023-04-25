#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.


class TensorInfoDto:
    def __init__(self: any) -> None:
        self._level = None
        self._type = None
        self._thread_id = None
        self._data_len = None
        self._timestamp = None
        self._node_id = None
        self._tensor_num = None
        self._input_formats = None
        self._input_data_types = None
        self._input_shapes = None
        self._output_formats = None
        self._output_data_types = None
        self._output_shapes = None

    @property
    def level(self):
        return self._level

    @level.setter
    def level(self, value):
        self._level = value

    @property
    def type(self):
        return self._type

    @type.setter
    def type(self, value):
        self._type = value

    @property
    def thread_id(self):
        return self._thread_id

    @thread_id.setter
    def thread_id(self, value):
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
    def tensor_num(self):
        return self._tensor_num

    @tensor_num.setter
    def tensor_num(self, value):
        self._tensor_num = value

    @property
    def input_formats(self):
        return self._input_formats

    @input_formats.setter
    def input_formats(self, value):
        self._input_formats = value

    @property
    def input_data_types(self):
        return self._input_data_types

    @input_data_types.setter
    def input_data_types(self, value):
        self._input_data_types = value

    @property
    def input_shapes(self):
        return self._input_shapes

    @input_shapes.setter
    def input_shapes(self, value):
        self._input_shapes = value

    @property
    def output_formats(self):
        return self._output_formats

    @output_formats.setter
    def output_formats(self, value):
        self._output_formats = value

    @property
    def output_data_types(self):
        return self._output_data_types

    @output_data_types.setter
    def output_data_types(self, value):
        self._output_data_types = value

    @property
    def output_shapes(self):
        return self._output_shapes

    @output_shapes.setter
    def output_shapes(self, value):
        self._output_shapes = value

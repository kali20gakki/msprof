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

    @property
    def type(self):
        return self._type

    @property
    def thread_id(self):
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
    def tensor_num(self):
        return self._tensor_num

    @property
    def input_formats(self):
        return self._input_formats

    @property
    def input_data_types(self):
        return self._input_data_types

    @property
    def input_shapes(self):
        return self._input_shapes

    @property
    def output_formats(self):
        return self._output_formats

    @property
    def output_data_types(self):
        return self._output_data_types

    @property
    def output_shapes(self):
        return self._output_shapes

    @level.setter
    def level(self, value):
        self._level = value

    @type.setter
    def type(self, value):
        self._type = value

    @thread_id.setter
    def thread_id(self, value):
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

    @tensor_num.setter
    def tensor_num(self, value):
        self._tensor_num = value

    @input_formats.setter
    def input_formats(self, value):
        self._input_formats = value

    @input_data_types.setter
    def input_data_types(self, value):
        self._input_data_types = value

    @input_shapes.setter
    def input_shapes(self, value):
        self._input_shapes = value

    @output_formats.setter
    def output_formats(self, value):
        self._output_formats = value

    @output_data_types.setter
    def output_data_types(self, value):
        self._output_data_types = value

    @output_shapes.setter
    def output_shapes(self, value):
        self._output_shapes = value

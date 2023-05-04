#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.


class GraphIdMapDto:
    def __init__(self: any) -> None:
        self._level = None
        self._struct_type = None
        self._thread_id = None
        self._data_len = None
        self._timestamp = None
        self._graph_id = None
        self._model_name = None

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
    def timestamp(self: any) -> int:
        return self._timestamp

    @property
    def graph_id(self):
        return self._graph_id

    @property
    def model_name(self):
        return self._model_name

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
    def timestamp(self: any, value: any) -> None:
        self._timestamp = value

    @graph_id.setter
    def graph_id(self, value):
        self._graph_id = value

    @model_name.setter
    def model_name(self, value):
        self._model_name = value

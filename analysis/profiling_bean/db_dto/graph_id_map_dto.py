#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.


class GraphIdMapDto:
    def __init__(self: any) -> None:
        self._level = None
        self._type = None
        self._thread_id = None
        self._data_len = None
        self._timestamp = None
        self._graph_id = None
        self._model_name = None

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
    def timestamp(self: any) -> int:
        return self._timestamp

    @timestamp.setter
    def timestamp(self: any, value: any) -> None:
        self._timestamp = value

    @property
    def graph_id(self):
        return self._graph_id

    @graph_id.setter
    def graph_id(self, value):
        self._graph_id = value

    @property
    def model_name(self):
        return self._model_name

    @model_name.setter
    def model_name(self, value):
        self._model_name = value

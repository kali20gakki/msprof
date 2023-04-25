#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.


class MultiThreadDto:
    def __init__(self: any) -> None:
        self._level = None
        self._type = None
        self._thread_id = None
        self._data_len = None
        self._timestamp = None
        self._thread_num = None
        self._item_id = None

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
    def thread_num(self):
        return self._thread_num

    @thread_num.setter
    def thread_num(self, value):
        self._thread_num = value

    @property
    def item_id(self):
        return self._item_id

    @item_id.setter
    def item_id(self, value):
        self._item_id = value

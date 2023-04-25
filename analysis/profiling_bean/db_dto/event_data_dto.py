#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.


class EventDataDto:
    def __init__(self: any) -> None:
        self._level = None
        self._type = None
        self._thread_id = None
        self._request_id = None
        self._timestamp = None
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
    def request_id(self):
        return self._request_id

    @request_id.setter
    def request_id(self, value):
        self._request_id = value

    @property
    def timestamp(self: any) -> int:
        return self._timestamp

    @timestamp.setter
    def timestamp(self: any, value: any) -> None:
        self._timestamp = value

    @property
    def item_id(self: any) -> int:
        return self._item_id

    @item_id.setter
    def item_id(self: any, value: any) -> None:
        self._item_id = value

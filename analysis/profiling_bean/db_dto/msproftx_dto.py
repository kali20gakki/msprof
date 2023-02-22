#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.

from common_func.constant import Constant


class MsprofTxDto:
    """
    class used to decode binary data
    """
    CHAR_LIST_SIZE = 128

    def __init__(self: any, *args: list) -> None:
        self._pid = 0
        self._tid = 0
        self._category = 0
        self._event_type = 0
        self._payload_type = 0
        self._payload_value = 0
        self._start_time = 0
        self._end_time = 0
        self._message_type = 0
        self._message = Constant.NA
        self._call_stack = Constant.NA
        self._dur_time = 0
        self._file_tag = Constant.DEFAULT_INVALID_VALUE

    @property
    def pid(self: any) -> int:
        return self._pid

    @property
    def tid(self: any) -> int:
        return self._tid

    @property
    def category(self: any) -> int:
        return self._category

    @property
    def event_type(self: any) -> int:
        return self._event_type

    @property
    def payload_type(self: any) -> int:
        return self._payload_type

    @property
    def payload_value(self: any) -> int:
        return self._payload_value

    @property
    def start_time(self: any) -> int:
        return self._start_time

    @property
    def end_time(self: any) -> int:
        return self._end_time

    @property
    def message_type(self: any) -> int:
        return self._message_type

    @property
    def message(self: any) -> str:
        return self._message

    @property
    def call_stack(self: any) -> str:
        return self._call_stack

    @property
    def dur_time(self: any) -> int:
        return self._dur_time

    @pid.setter
    def pid(self: any, value: int) -> None:
        self._pid = value

    @tid.setter
    def tid(self: any, value: int) -> None:
        self._tid = value

    @category.setter
    def category(self: any, value: int) -> None:
        self._category = value

    @event_type.setter
    def event_type(self: any, value: int) -> None:
        self._event_type = value

    @payload_type.setter
    def payload_type(self: any, value: int) -> None:
        self._payload_type = value

    @payload_value.setter
    def payload_value(self: any, value: int) -> None:
        self._payload_value = value

    @start_time.setter
    def start_time(self: any, value: int) -> None:
        self._start_time = value

    @end_time.setter
    def end_time(self: any, value: int) -> None:
        self._end_time = value

    @message_type.setter
    def message_type(self: any, value: int) -> None:
        self._message_type = value

    @message.setter
    def message(self: any, value: str) -> None:
        self._message = value

    @call_stack.setter
    def call_stack(self: any, value: str) -> None:
        self._call_stack = value

    @dur_time.setter
    def dur_time(self: any, value: int) -> None:
        self._dur_time = value

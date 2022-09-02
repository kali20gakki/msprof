#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2022. All rights reserved.
"""


class GeOpExecuteDto:
    """
    Dto for ge op execute data
    """

    def __init__(self: any) -> None:
        self._thread_id = None
        self._op_type = None
        self._event_type = None
        self._start_time = None
        self._end_time = None

    @property
    def thread_id(self: any) -> any:
        return self._thread_id

    @thread_id.setter
    def thread_id(self: any, value: any) -> None:
        self._thread_id = value

    @property
    def op_type(self: any) -> any:
        return self._op_type

    @op_type.setter
    def op_type(self: any, value: any) -> None:
        self._op_type = value

    @property
    def event_type(self: any) -> any:
        return self._event_type

    @event_type.setter
    def event_type(self: any, value: any) -> None:
        self._event_type = value

    @property
    def start_time(self: any) -> any:
        return self._start_time

    @start_time.setter
    def start_time(self: any, value: any) -> None:
        self._start_time = value

    @property
    def end_time(self: any) -> any:
        return self._end_time

    @end_time.setter
    def end_time(self: any, value: any) -> None:
        self._end_time = value

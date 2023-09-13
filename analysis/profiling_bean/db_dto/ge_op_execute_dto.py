#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.


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

    @property
    def op_type(self: any) -> any:
        return self._op_type

    @property
    def event_type(self: any) -> any:
        return self._event_type

    @property
    def start_time(self: any) -> any:
        return self._start_time

    @property
    def end_time(self: any) -> any:
        return self._end_time

    @thread_id.setter
    def thread_id(self: any, value: any) -> None:
        self._thread_id = value

    @op_type.setter
    def op_type(self: any, value: any) -> None:
        self._op_type = value

    @event_type.setter
    def event_type(self: any, value: any) -> None:
        self._event_type = value

    @start_time.setter
    def start_time(self: any, value: any) -> None:
        self._start_time = value

    @end_time.setter
    def end_time(self: any, value: any) -> None:
        self._end_time = value

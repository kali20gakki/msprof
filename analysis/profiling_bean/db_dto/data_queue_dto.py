#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

class DataQueueDto:

    def __init__(self: any):
        self._node_name = None
        self._queue_size = None
        self._start_time = None
        self._end_time = None
        self._duration = None

    @property
    def node_name(self: any) -> any:
        return self._node_name

    @property
    def queue_size(self: any) -> any:
        return self._queue_size

    @property
    def start_time(self: any) -> any:
        return self._start_time

    @property
    def end_time(self: any) -> any:
        return self._end_time

    @property
    def duration(self: any) -> any:
        return self._duration

    @duration.setter
    def duration(self: any, value: any) -> None:
        self._duration = value

    @node_name.setter
    def node_name(self: any, value: any) -> None:
        self._node_name = value

    @queue_size.setter
    def queue_size(self: any, value: any) -> None:
        self._queue_size = value

    @end_time.setter
    def end_time(self: any, value: any) -> None:
        self._end_time = value

    @start_time.setter
    def start_time(self: any, value: any) -> None:
        self._start_time = value

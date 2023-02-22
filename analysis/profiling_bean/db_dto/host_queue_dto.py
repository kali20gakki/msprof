#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

class HostQueueDto:

    def __init__(self: any):
        self._index_id = None
        self._queue_capacity = None
        self._queue_size = None
        self._mode = None
        self._get_time = None
        self._send_time = None
        self._total_time = None

    @property
    def index_id(self: any) -> any:
        return self._index_id

    @property
    def queue_capacity(self: any) -> any:
        return self._queue_capacity

    @property
    def queue_size(self: any) -> any:
        return self._queue_size

    @property
    def mode(self: any) -> any:
        return self._mode

    @property
    def get_time(self: any) -> any:
        return self._get_time

    @property
    def send_time(self: any) -> any:
        return self._send_time

    @property
    def total_time(self: any) -> any:
        return self._total_time

    @index_id.setter
    def index_id(self: any, value: any) -> None:
        self._index_id = value

    @queue_capacity.setter
    def queue_capacity(self: any, value: any) -> None:
        self._queue_capacity = value

    @queue_size.setter
    def queue_size(self: any, value: any) -> None:
        self._queue_size = value

    @mode.setter
    def mode(self: any, value: any) -> None:
        self._mode = value

    @get_time.setter
    def get_time(self: any, value: any) -> None:
        self._get_time = value

    @send_time.setter
    def send_time(self: any, value: any) -> None:
        self._send_time = value

    @total_time.setter
    def total_time(self: any, value: any) -> None:
        self._total_time = value

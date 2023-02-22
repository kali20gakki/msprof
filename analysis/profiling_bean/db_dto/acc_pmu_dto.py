#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.


class AccPmuOriDto:
    """
    Dto for acc pmu data
    """

    def __init__(self: any) -> None:
        self._task_id = None
        self._stream_id = None
        self._acc_id = None
        self._block_id = None
        self._read_bandwidth = None
        self._write_bandwidth = None
        self._read_ost = None
        self._write_ost = None
        self._timestamp = None

    @property
    def task_id(self: any) -> any:
        return self._task_id

    @property
    def stream_id(self: any) -> any:
        return self._stream_id

    @property
    def acc_id(self: any) -> any:
        return self._acc_id

    @property
    def block_id(self: any) -> any:
        return self._block_id

    @property
    def read_bandwidth(self: any) -> any:
        return self._read_bandwidth

    @property
    def write_bandwidth(self: any) -> any:
        return self._write_bandwidth

    @property
    def read_ost(self: any) -> any:
        return self._read_ost

    @property
    def write_ost(self: any) -> any:
        return self._write_ost

    @property
    def timestamp(self: any) -> any:
        return self._timestamp

    @task_id.setter
    def task_id(self: any, value: any) -> None:
        self._task_id = value

    @stream_id.setter
    def stream_id(self: any, value: any) -> None:
        self._stream_id = value

    @acc_id.setter
    def acc_id(self: any, value: any) -> None:
        self._acc_id = value

    @block_id.setter
    def block_id(self: any, value: any) -> None:
        self._block_id = value

    @read_bandwidth.setter
    def read_bandwidth(self: any, value: any) -> None:
        self._read_bandwidth = value

    @write_bandwidth.setter
    def write_bandwidth(self: any, value: any) -> None:
        self._write_bandwidth = value

    @read_ost.setter
    def read_ost(self: any, value: any) -> None:
        self._read_ost = value

    @write_ost.setter
    def write_ost(self: any, value: any) -> None:
        self._write_ost = value

    @timestamp.setter
    def timestamp(self: any, value: any) -> None:
        self._timestamp = value

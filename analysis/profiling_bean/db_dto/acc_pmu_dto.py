#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2022. All rights reserved.
"""


class AccPmuOriDto:
    """
    Dto for acc pmu data
    """

    def __init__(self: any) -> None:
        self._task_id = None
        self._stream_id = None
        self._acc_id = None
        self._block_id = None
        self._stream_id = None
        self._read_bandwidth = None
        self._write_bandwidth = None
        self._read_ost = None
        self._write_ost = None
        self._timestamp = None

    @property
    def task_id(self: any) -> any:
        return self._task_id

    @task_id.setter
    def task_id(self: any, value: any) -> None:
        self._task_id = value

    @property
    def stream_id(self: any) -> any:
        return self._stream_id

    @stream_id.setter
    def stream_id(self: any, value: any) -> None:
        self._stream_id = value

    @property
    def acc_id(self: any) -> any:
        return self._acc_id

    @acc_id.setter
    def acc_id(self: any, value: any) -> None:
        self._acc_id = value

    @property
    def block_id(self: any) -> any:
        return self._block_id

    @block_id.setter
    def block_id(self: any, value: any) -> None:
        self._block_id = value

    @property
    def read_bandwidth(self: any) -> any:
        return self._read_bandwidth

    @read_bandwidth.setter
    def read_bandwidth(self: any, value: any) -> None:
        self._read_bandwidth = value

    @property
    def write_bandwidth(self: any) -> any:
        return self._write_bandwidth

    @write_bandwidth.setter
    def write_bandwidth(self: any, value: any) -> None:
        self._write_bandwidth = value

    @property
    def read_ost(self: any) -> any:
        return self._read_ost

    @read_ost.setter
    def read_ost(self: any, value: any) -> None:
        self._read_ost = value

    @property
    def write_ost(self: any) -> any:
        return self._write_ost

    @write_ost.setter
    def write_ost(self: any, value: any) -> None:
        self._write_ost = value

    @property
    def timestamp(self: any) -> any:
        return self._timestamp

    @timestamp.setter
    def timestamp(self: any, value: any) -> None:
        self._timestamp = value

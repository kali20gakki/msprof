#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

from profiling_bean.struct_info.struct_decoder import StructDecoder


class RtsDataBean(StructDecoder):
    """
    rts track bean data for the data parsing by rts parser.
    """

    def __init__(self: any, *args: any) -> None:
        _rts_data = args[0]
        self._thread_id = _rts_data[2]
        self._timestamp = _rts_data[3]
        self._tasktype = _rts_data[4].partition(b'\x00')[0].decode('utf-8', 'ignore')
        self._task_id = _rts_data[5]
        self._batch_id = _rts_data[6]
        self._stream_id = _rts_data[7]

    @property
    def timestamp(self: any) -> int:
        """
        rts timestamp
        :return:
        """
        return self._timestamp

    @property
    def task_type(self: any) -> int:
        """
        rts task_type
        :return:
        """
        return self._tasktype

    @property
    def stream_id(self: any) -> int:
        """
        rts stream_id
        :return:
        """
        return self._stream_id

    @property
    def batch_id(self: any) -> int:
        """
        rts batch_id
        :return:
        """
        return self._batch_id

    @property
    def task_id(self: any) -> int:
        """
        rts task_id
        :return:
        """
        return self._task_id

    @property
    def thread_id(self: any) -> int:
        """
        rts thread_id
        :return:
        """
        return self._thread_id

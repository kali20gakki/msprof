#!/usr/bin/env python
# coding=utf-8
"""
function:Ge host bean struct
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
from profiling_bean.struct_info.struct_decoder import StructDecoder


class GeHostBean(StructDecoder):
    """
    bean for Ge host data
    """

    def __init__(self: any, *args: any) -> None:
        _data = args[0]
        self._magic_num = _data[0]
        self._data_tag = _data[1]
        self._thread_id = _data[2]
        self._op_type = str(_data[3])
        self._event_type = str(_data[4])
        self._start_time = _data[5]
        self._end_time = _data[6]

    @property
    def thread_id(self: any) -> int:
        """
        thread id
        :return: None
        """
        return self._thread_id

    @property
    def op_type(self: any) -> str:
        """
        object of event
        :return: None
        """
        return self._op_type

    @property
    def event_type(self: any) -> str:
        """
        event type
        :return: event
        """
        return self._event_type

    @property
    def start_time(self: any) -> int:
        """
        start time for the event
        :return: start time
        """
        return self._start_time

    @property
    def end_time(self: any) -> int:
        """
        end time for the event
        :return: end time
        """
        return self._end_time

    def check_data_complete(self: any) -> None:
        """
        check data for ge op execute bean
        :return: complete or not
        """
        return self._magic_num == 23130 and self._data_tag == 27

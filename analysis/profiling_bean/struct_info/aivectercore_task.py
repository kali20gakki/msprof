#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.

from profiling_bean.struct_info.struct_decoder import StructDecoder


class AiVecterCore(StructDecoder):
    """
    class used to decode ai_core task
    """

    def __init__(self: any, *args: any) -> None:
        args = args[0]
        # get lower 6 bit
        self._mode = args[0]
        # get high 6 bit
        self._event_count = args[5:13]
        self._task_cyc = args[13]
        self._timestamp = args[14]
        self._count_num = args[15]
        self._core_id = args[16]

    @property
    def event_count(self: any) -> int:
        """
        get event count
        :return: event count
        """
        return self._event_count

    @property
    def mode(self: any) -> str:
        """
        get mode
        :return: mode
        """
        return self._mode

    @property
    def timestamp(self: any) -> int:
        """
        get timestamp
        :return: timestamp
        """
        return self._timestamp

    @property
    def task_cyc(self: any) -> int:
        """
        get task cyc
        :return: task cyc
        """
        return self._task_cyc

    @property
    def core_id(self: any) -> int:
        """
        get core id
        :return: core_id
        """
        return self._core_id

    @property
    def count_num(self: any) -> int:
        """
        get count_num
        :return: count_num
        """
        return self._count_num

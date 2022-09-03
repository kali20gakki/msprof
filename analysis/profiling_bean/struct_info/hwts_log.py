#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.

from profiling_bean.struct_info.struct_decoder import StructDecoder


class HwtsLogBean(StructDecoder):
    """
    class used to decode hwts log from bytes
    """

    def __init__(self: any, *args: any) -> None:
        # 3=0b00000011, keep 2 lower bits which represent log type
        args = args[0]
        self._task_type = args[1]
        self._stream_id = args[6]
        self._task_id = args[4]
        self._sys_cnt = args[5]
        self._sys_tag = args[0] & 7

    @property
    def task_type(self: any) -> int:
        """
        for task type
        """
        return self._task_type

    @property
    def sys_tag(self: any) -> int:
        """
        for sys tag
        """
        return self._sys_tag

    @property
    def task_id(self: any) -> int:
        """
        for task id
        """
        return self._task_id

    @property
    def stream_id(self: any) -> int:
        """
        for stream id
        """
        return self._stream_id

    @property
    def sys_cnt(self: any) -> int:
        """
        for sys cnt
        """
        return self._sys_cnt

    def is_log_type(self: any) -> bool:
        """
        check hwts log type ,and 0 is start log ,1 is end log.
        :return:
        """
        return self._sys_tag in (0, 1)

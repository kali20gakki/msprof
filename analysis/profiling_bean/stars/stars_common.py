#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
from common_func.info_conf_reader import InfoConfReader


class StarsCommon:
    """
    class for decode binary data
    """

    def __init__(self: any, task_id: int, stream_id: int, timestamp: int or float) -> None:
        self._task_id = task_id
        self._stream_id = stream_id
        self._timestamp = timestamp

    @property
    def task_id(self: any) -> int:
        """
        get task id
        :return: task id
        """
        return self._task_id

    @property
    def stream_id(self: any) -> int:
        """
        get stream id
        :return: stream id
        """
        return self._stream_id

    @property
    def timestamp(self: any) -> float:
        """
        get timestamp
        :return: timestamp
        """
        return InfoConfReader().time_from_syscnt(self._timestamp)

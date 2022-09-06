#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.


class StarsCommon:
    """
    class for decode binary data
    """

    def __init__(self: any, task_id: int, stream_id: int, timestamp: int) -> None:
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
    def timestamp(self: any) -> int:
        """
        get timestamp
        :return: timestamp
        """
        return self._timestamp

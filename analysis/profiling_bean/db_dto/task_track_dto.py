#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
from profiling_bean.db_dto.event_data_dto import EventDataDto


class TaskTrackDto:
    def __init__(self: any):
        self._type = None
        self._level = None
        self._thread_id = None
        self._data_len = None
        self._timestamp = None
        self._stream_id = None
        self._device_id = None
        self._task_id = None
        self._batch_id = None

    @property
    def type(self):
        return self._type

    @type.setter
    def type(self, value):
        self._type = value

    @property
    def level(self):
        return self._level

    @level.setter
    def level(self, value):
        self._level = value

    @property
    def thread_id(self):
        return self._thread_id

    @thread_id.setter
    def thread_id(self, value):
        self._thread_id = value

    @property
    def data_len(self):
        return self._data_len

    @data_len.setter
    def data_len(self, value):
        self._data_len = value

    @property
    def timestamp(self):
        return self._timestamp

    @timestamp.setter
    def timestamp(self, value):
        self._timestamp = value

    @property
    def stream_id(self):
        return self._stream_id

    @stream_id.setter
    def stream_id(self, value):
        self._stream_id = value

    @property
    def device_id(self):
        return self._device_id

    @device_id.setter
    def device_id(self, value):
        self._device_id = value

    @property
    def task_id(self):
        return self._task_id

    @task_id.setter
    def task_id(self, value):
        self._task_id = value

    @property
    def batch_id(self):
        return self._batch_id

    @batch_id.setter
    def batch_id(self, value):
        self._batch_id = value


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
    def data_type(self):
        return self._type

    @property
    def level(self):
        return self._level

    @property
    def thread_id(self):
        return self._thread_id

    @property
    def data_len(self):
        return self._data_len

    @property
    def timestamp(self):
        return self._timestamp

    @property
    def stream_id(self):
        return self._stream_id

    @property
    def device_id(self):
        return self._device_id

    @property
    def task_id(self):
        return self._task_id

    @property
    def batch_id(self):
        return self._batch_id

    @data_type.setter
    def data_type(self, value):
        self._type = value

    @level.setter
    def level(self, value):
        self._level = value

    @thread_id.setter
    def thread_id(self, value):
        self._thread_id = value

    @data_len.setter
    def data_len(self, value):
        self._data_len = value

    @timestamp.setter
    def timestamp(self, value):
        self._timestamp = value

    @stream_id.setter
    def stream_id(self, value):
        self._stream_id = value

    @device_id.setter
    def device_id(self, value):
        self._device_id = value

    @task_id.setter
    def task_id(self, value):
        self._task_id = value

    @batch_id.setter
    def batch_id(self, value):
        self._batch_id = value

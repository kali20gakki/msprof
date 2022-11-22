#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.

from common_func.constant import Constant


class StepTraceGeDto:
    def __init__(self: any) -> None:
        self._fusion_data = ()
        self._model_id = Constant.DEFAULT_INVALID_VALUE
        self._index_id = Constant.DEFAULT_INVALID_VALUE
        self._stream_id = Constant.DEFAULT_INVALID_VALUE
        self._task_id = Constant.DEFAULT_INVALID_VALUE
        self._timestamp = Constant.DEFAULT_INVALID_VALUE
        self._tag_id = Constant.DEFAULT_INVALID_VALUE
        self._op_name = Constant.NA
        self._op_type = Constant.NA

    @property
    def model_id(self: any) -> any:
        """
        for model id
        """
        return self._model_id

    @model_id.setter
    def model_id(self: any, value: any) -> None:
        self._model_id = value

    @property
    def index_id(self: any) -> any:
        """
        for index id
        """
        return self._index_id

    @index_id.setter
    def index_id(self: any, value: any) -> None:
        self._index_id = value

    @property
    def stream_id(self: any) -> any:
        """
        for stream id
        """
        return self._stream_id

    @stream_id.setter
    def stream_id(self: any, value: any) -> None:
        self._stream_id = value

    @property
    def task_id(self: any) -> any:
        """
        for task id
        """
        return self._task_id

    @task_id.setter
    def task_id(self: any, value: any) -> None:
        self._task_id = value

    @property
    def timestamp(self: any) -> any:
        """
        for timestamp
        """
        return self._timestamp

    @timestamp.setter
    def timestamp(self: any, value: any) -> None:
        self._timestamp = value

    @property
    def tag_id(self: any) -> any:
        """
        for tag
        """
        return self._tag_id

    @tag_id.setter
    def tag_id(self: any, value: any) -> None:
        self._tag_id = value

    @property
    def op_name(self: any) -> any:
        """
        for op name
        """
        return self._op_name

    @op_name.setter
    def op_name(self: any, value: any) -> None:
        self._op_name = value

    @property
    def op_type(self: any) -> any:
        """
        for op type
        """
        return self._op_type

    @op_type.setter
    def op_type(self: any, value: any) -> None:
        self._op_type = value

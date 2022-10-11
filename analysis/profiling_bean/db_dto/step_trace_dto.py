#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.


class StepTraceDto:
    """
    step trace dto
    """
    DEFAULT_ITER_ID = -1

    def __init__(self: any) -> None:
        self._index_id = None
        self._model_id = None
        self._step_start = None
        self._step_end = None
        self._iter_id = None

    @property
    def index_id(self: any) -> any:
        return self._index_id

    @index_id.setter
    def index_id(self: any, value: any) -> None:
        self._index_id = value

    @property
    def model_id(self: any) -> any:
        return self._model_id

    @model_id.setter
    def model_id(self: any, value: any) -> None:
        self._model_id = value

    @property
    def step_start(self: any) -> any:
        return self._step_start

    @step_start.setter
    def step_start(self: any, value: any) -> None:
        self._step_start = value

    @property
    def step_end(self: any) -> any:
        return self._step_end

    @step_end.setter
    def step_end(self: any, value: any) -> None:
        self._step_end = value

    @property
    def iter_id(self: any) -> any:
        return self._iter_id

    @iter_id.setter
    def iter_id(self: any, value: any) -> None:
        self._iter_id = value

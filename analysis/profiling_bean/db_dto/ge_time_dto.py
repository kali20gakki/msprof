#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.


class GeTimeDto:
    """
    Dto for ge model time data
    """

    def __init__(self: any) -> None:
        self._model_name = None
        self._model_id = None
        self._thread_id = None
        self._input_start = None
        self._input_end = None
        self._infer_start = None
        self._infer_end = None
        self._output_start = None
        self._output_end = None

    @property
    def model_name(self: any) -> any:
        return self._model_name

    @model_name.setter
    def model_name(self: any, value: any) -> None:
        self._model_name = value

    @property
    def model_id(self: any) -> any:
        return self._model_id

    @model_id.setter
    def model_id(self: any, value: any) -> None:
        self._model_id = value

    @property
    def thread_id(self: any) -> any:
        return self._thread_id

    @thread_id.setter
    def thread_id(self: any, value: any) -> None:
        self._thread_id = value

    @property
    def input_start(self: any) -> any:
        return self._input_start

    @input_start.setter
    def input_start(self: any, value: any) -> None:
        self._input_start = value

    @property
    def input_end(self: any) -> any:
        return self._input_end

    @input_end.setter
    def input_end(self: any, value: any) -> None:
        self._input_end = value

    @property
    def infer_start(self: any) -> any:
        return self._infer_start

    @infer_start.setter
    def infer_start(self: any, value: any) -> None:
        self._infer_start = value

    @property
    def infer_end(self: any) -> any:
        return self._infer_end

    @infer_end.setter
    def infer_end(self: any, value: any) -> None:
        self._infer_end = value

    @property
    def output_start(self: any) -> any:
        return self._output_start

    @output_start.setter
    def output_start(self: any, value: any) -> None:
        self._output_start = value

    @property
    def output_end(self: any) -> any:
        return self._output_end

    @output_end.setter
    def output_end(self: any, value: any) -> None:
        self._output_end = value

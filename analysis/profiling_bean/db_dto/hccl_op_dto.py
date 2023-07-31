#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
class HCCLOpDto:
    def __init__(self):
        self.model_id = None
        self.index_id = None
        self.thread_id = None
        self.op_name = None
        self.start = None
        self.end = None

    @property
    def model_id(self):
        return self._model_id

    @property
    def index_id(self):
        return self._index_id

    @property
    def thread_id(self):
        return self._thread_id

    @property
    def op_name(self):
        return self._op_name

    @property
    def start(self):
        return self._start

    @property
    def end(self):
        return self._end

    @model_id.setter
    def model_id(self, value):
        self._model_id = value

    @index_id.setter
    def index_id(self, value):
        self._index_id = value

    @thread_id.setter
    def thread_id(self, value):
        self._thread_id = value

    @op_name.setter
    def op_name(self, value):
        self._op_name = value

    @start.setter
    def start(self, value):
        self._start = value

    @end.setter
    def end(self, value):
        self._end = value
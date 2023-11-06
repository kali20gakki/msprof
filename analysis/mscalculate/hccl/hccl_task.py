#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

import ast
from dataclasses import dataclass

from common_func.constant import Constant


class DictConversionDescriptor:
    def __init__(self, *, default):
        self._default = default
        self._name = ""

    def __set_name__(self, owner, name):
        self._name = "_" + name

    def __get__(self, obj, obj_type):
        if obj is None:
            return self._default
        return getattr(obj, self._name, self._default)

    def __set__(self, obj, value):
        setattr(obj, self._name, ast.literal_eval(value))


@dataclass
class HcclOps:
    device_id: int = Constant.DEFAULT_VALUE
    model_id: int = Constant.DEFAULT_INVALID_VALUE
    index_id: int = Constant.DEFAULT_INVALID_VALUE
    thread_id: int = Constant.DEFAULT_INVALID_VALUE
    op_name: str = Constant.NA
    task_type: str = Constant.NA
    op_type: str = Constant.NA
    timestamp: int = Constant.DEFAULT_VALUE
    duration: int = Constant.DEFAULT_VALUE
    begin: int = Constant.DEFAULT_VALUE
    end: int = Constant.DEFAULT_VALUE
    is_dynamic: int = Constant.DEFAULT_INVALID_VALUE
    connection_id: int = Constant.DEFAULT_INVALID_VALUE


@dataclass
class HcclTask:
    model_id: int = Constant.DEFAULT_INVALID_VALUE
    index_id: int = Constant.DEFAULT_INVALID_VALUE
    name: str = Constant.NA
    group_name: str = Constant.NA
    plane_id: int = Constant.DEFAULT_VALUE
    timestamp: int = Constant.DEFAULT_VALUE
    duration: int = Constant.DEFAULT_VALUE
    stream_id: int = Constant.DEFAULT_VALUE
    task_id: int = Constant.DEFAULT_VALUE
    context_id: int = Constant.DEFAULT_VALUE
    batch_id: int = Constant.DEFAULT_VALUE
    iteration: int = Constant.DEFAULT_VALUE
    hccl_name: str = Constant.NA
    first_timestamp: int = Constant.DEFAULT_VALUE
    host_timestamp: int = Constant.DEFAULT_INVALID_VALUE
    device_id: int = Constant.DEFAULT_VALUE
    args: DictConversionDescriptor = DictConversionDescriptor(default="{}")
    is_dynamic: int = Constant.DEFAULT_INVALID_VALUE
    is_master: int = Constant.DEFAULT_INVALID_VALUE
    op_name: str = Constant.NA
    op_type: str = Constant.NA
    task_type: str = Constant.NA
    connection_id: int = Constant.DEFAULT_INVALID_VALUE


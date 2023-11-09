#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

import ast
from dataclasses import dataclass
from collections import OrderedDict
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
    DEFAULT_REFLECTOR = {
        "device_id": Constant.DEFAULT_VALUE,
        "model_id": Constant.DEFAULT_INVALID_VALUE,
        "index_id": Constant.DEFAULT_INVALID_VALUE,
        "thread_id": Constant.DEFAULT_INVALID_VALUE,
        "op_name": Constant.NA,
        "task_type": Constant.NA,
        "op_type": Constant.NA,
        "timestamp": Constant.DEFAULT_VALUE,
        "duration": Constant.DEFAULT_VALUE,
        "begin": Constant.DEFAULT_VALUE,
        "end": Constant.DEFAULT_VALUE,
        "is_dynamic": Constant.DEFAULT_INVALID_VALUE,
        "connection_id": Constant.DEFAULT_INVALID_VALUE
    }
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
    DEFAULT_REFLECTOR = {
        "model_id":  Constant.DEFAULT_INVALID_VALUE,
        "index_id": Constant.DEFAULT_INVALID_VALUE,
        "name": Constant.NA,
        "group_name": Constant.NA,
        "plane_id": Constant.DEFAULT_VALUE,
        "timestamp": Constant.DEFAULT_VALUE,
        "duration": Constant.DEFAULT_VALUE,
        "stream_id": Constant.DEFAULT_VALUE,
        "task_id": Constant.DEFAULT_VALUE,
        "context_id": Constant.DEFAULT_VALUE,
        "batch_id": Constant.DEFAULT_VALUE,
        "iteration": Constant.DEFAULT_VALUE,
        "hccl_name": Constant.NA,
        "first_timestamp": Constant.DEFAULT_VALUE,
        "host_timestamp": Constant.DEFAULT_INVALID_VALUE,
        "device_id": Constant.DEFAULT_VALUE,
        "args": DictConversionDescriptor(default="{}"),
        "is_dynamic": Constant.DEFAULT_INVALID_VALUE,
        "op_name": Constant.NA,
        "op_type": Constant.NA,
        "task_type": Constant.NA,
        "connection_id": Constant.DEFAULT_INVALID_VALUE,
        "struct_type": Constant.DEFAULT_INVALID_VALUE,
        "is_master": Constant.DEFAULT_INVALID_VALUE,
        "duration_estimated": Constant.DEFAULT_INVALID_VALUE,
        "local_rank": Constant.NA,
        "remote_rank": Constant.NA,
        "transport_type": Constant.NA,
        "data_type": Constant.NA,
        "link_type": Constant.NA,
        "size": Constant.DEFAULT_VALUE,
        "bandwidth": Constant.DEFAULT_INVALID_VALUE
    }
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
    is_dynamic: int = Constant.DEFAULT_INVALID_VALUE
    is_master: int = Constant.DEFAULT_INVALID_VALUE
    op_name: str = Constant.NA
    op_type: str = Constant.NA
    task_type: str = Constant.NA
    connection_id: int = Constant.DEFAULT_INVALID_VALUE
    is_master: int = Constant.DEFAULT_INVALID_VALUE
    struct_type: int = Constant.DEFAULT_INVALID_VALUE
    duration_estimated: int = Constant.DEFAULT_INVALID_VALUE
    local_rank: int = Constant.DEFAULT_INVALID_VALUE
    remote_rank: int = Constant.DEFAULT_INVALID_VALUE
    transport_type: str = Constant.NA
    data_type: str = Constant.NA
    link_type: str = Constant.NA
    size: int = Constant.NA
    bandwidth: int = Constant.DEFAULT_INVALID_VALUE
    notify_id: int = Constant.DEFAULT_INVALID_VALUE

#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
from dataclasses import dataclass
from profiling_bean.db_dto.dto_meta_class import InstanceCheckMeta


@dataclass
class TaskTrackDto(metaclass=InstanceCheckMeta):
    batch_id = None
    data_len = None
    device_id = None
    level = None
    stream_id = None
    struct_type = None
    task_id = None
    task_type = None
    thread_id = None
    timestamp = None

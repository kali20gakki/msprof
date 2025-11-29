#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
from dataclasses import dataclass

from profiling_bean.db_dto.dto_meta_class import InstanceCheckMeta


@dataclass
class PmuBlockDto(metaclass=InstanceCheckMeta):
    stream_id: int = None
    task_id: int = None
    subtask_id: int = None
    batch_id: int = None
    start_time: int = None
    duration: int = None
    core_type: int = None
    core_id: int = None
#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

from dataclasses import dataclass
from profiling_bean.db_dto.dto_meta_class import InstanceCheckMeta


@dataclass
class NpuOpMemDto(metaclass=InstanceCheckMeta):
    """
    Dto for npu op mem data
    """
    addr = None
    device_type = None
    level = None
    operator = None
    size = None
    thread_id = None
    timestamp = None
    total_allocate_memory = None
    total_reserve_memory = None
    type = None
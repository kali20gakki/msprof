#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

from dataclasses import dataclass
from profiling_bean.db_dto.dto_meta_class import InstanceCheckMeta


@dataclass
class NpuOpMemRecDto(metaclass=InstanceCheckMeta):
    """
    Dto for npu op mem data
    """
    component = None
    device_type = None
    timestamp = None
    total_allocate_memory = None
    total_reserve_memory = None

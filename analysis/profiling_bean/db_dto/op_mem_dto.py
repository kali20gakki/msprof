#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.


from dataclasses import dataclass
from profiling_bean.db_dto.dto_meta_class import InstanceCheckMeta


@dataclass
class OpMemDto(metaclass=InstanceCheckMeta):
    """
    Dto for npu op mem data
    """
    allocation_time = None
    allocation_total_allocated = None
    allocation_total_reserved = None
    device_type = None
    duration = None
    name = None
    operator = None
    release_time = None
    release_total_allocated = None
    release_total_reserved = None
    size = None
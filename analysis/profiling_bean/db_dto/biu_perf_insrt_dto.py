#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.

from dataclasses import dataclass
from profiling_bean.db_dto.dto_meta_class import InstanceCheckMeta


@dataclass
class BiuPerfInstrDto(metaclass=InstanceCheckMeta):
    """
    Dto for Biu Perf Insrt data
    """
    group_id: int
    core_type: str
    block_id: int
    instruction: str
    timestamp: int
    duration: int
    checkpoint_info: int

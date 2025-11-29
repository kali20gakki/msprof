#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.

from dataclasses import dataclass
from profiling_bean.db_dto.dto_meta_class import InstanceCheckMeta


@dataclass
class OriginMissionDto(metaclass=InstanceCheckMeta):
    """
    Dto for ccu Mission data
    """
    stream_id: int = None
    task_id: int = None
    lp_instr_id: int = None
    setckebit_instr_id: int = None
    rel_id: int = None
    start_time: float = None
    end_time: float = None
    time_type: str = None
    lp_start_time: float = None
    lp_end_time: float = None
    setckebit_start_time: float = None
    rel_end_time: float = None

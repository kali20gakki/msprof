#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.

from dataclasses import dataclass
from profiling_bean.db_dto.dto_meta_class import InstanceCheckMeta


@dataclass
class OriginChannelDto(metaclass=InstanceCheckMeta):
    """
    Dto for ccu Channel data
    """
    channel_id: int = None
    timestamp: float = None
    max_bw: int = None
    min_bw: int = None
    avg_bw: int = None

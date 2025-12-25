#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.

from dataclasses import dataclass
from profiling_bean.db_dto.dto_meta_class import InstanceCheckMeta


@dataclass
class OriginCCUAddInfoDto(metaclass=InstanceCheckMeta):
    """
    Dto for CCU Task Info data
    """
    version: int = None
    work_flow_mode: int = None
    item_id: str = None
    group_name: str = None
    rank_id: int = None
    rank_size: int = None
    stream_id: int = None
    task_id: int = None
    die_id: int = None
    mission_id: int = None
    instr_id: int = None


@dataclass
class OriginTaskInfoDto(OriginCCUAddInfoDto):
    """
    Dto for CCU Task Info data
    """


@dataclass
class OriginWaitSignalInfoDto(OriginCCUAddInfoDto):
    """
    Dto for CCU Wait Signal Info data
    """
    cke_id: int = None
    mask: int = None
    channel_id: int = None
    remote_rank_id: int = None


@dataclass
class OriginGroupInfoDto(OriginCCUAddInfoDto):
    """
    Dto for CCU Group Info data
    """
    reduce_op_type: str = None
    input_data_type: str = None
    output_data_type: str = None
    data_size: int = None
    channel_id: int = None
    remote_rank_id: int = None

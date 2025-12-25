#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
from dataclasses import dataclass

from common_func.ms_constant.number_constant import NumberConstant
from profiling_bean.db_dto.dto_meta_class import InstanceCheckMeta


@dataclass
class BlockLogDto(metaclass=InstanceCheckMeta):
    """
    Dto for stars block log data
    """
    stream_id: int
    task_id: int
    block_id: int
    context_id: int
    start_time: int
    duration: float
    device_task_type: str
    core_type: int
    core_id: int
    batch_id: int = NumberConstant.DEFAULT_BATCH_ID
    subtask_id: int = NumberConstant.DEFAULT_GE_CONTEXT_ID

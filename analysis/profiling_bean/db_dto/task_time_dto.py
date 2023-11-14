#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
from dataclasses import dataclass
from profiling_bean.db_dto.dto_meta_class import InstanceCheckMeta


@dataclass
class TaskTimeDto(metaclass=InstanceCheckMeta):
    """
    Dto for stars data or hwts data
    """
    batch_id = None
    dur_time = None
    end_time = None
    ffts_type = None
    op_name = None
    start_time = None
    stream_id = None
    subtask_id = None
    subtask_type = None
    task_id = None
    task_time = None
    task_type = None
    thread_id = None

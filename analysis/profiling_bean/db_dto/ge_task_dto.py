#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.


from dataclasses import dataclass
from profiling_bean.db_dto.dto_meta_class import InstanceCheckMeta


@dataclass
class GeTaskDto(metaclass=InstanceCheckMeta):
    batch_id = None
    block_dim = None
    context_id = None
    index_id = None
    mix_block_dim = None
    model_id = None
    op_name = None
    op_state = None
    op_type = None
    stream_id = None
    task_id = None
    task_type = None
    thread_id = None
    timestamp = None
#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

from dataclasses import dataclass
from profiling_bean.db_dto.dto_meta_class import InstanceCheckMeta


@dataclass
class NodeBasicInfoDto(metaclass=InstanceCheckMeta):
    block_dim = None
    data_len = None
    is_dynamic = None
    level = None
    mix_block_dim = None
    op_flag = None
    op_name = None
    op_type = None
    struct_type = None
    task_type = None
    thread_id = None
    timestamp = None
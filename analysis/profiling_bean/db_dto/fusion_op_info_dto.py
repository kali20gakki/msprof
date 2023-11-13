#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.


from dataclasses import dataclass
from profiling_bean.db_dto.dto_meta_class import InstanceCheckMeta


@dataclass
class FusionOpInfoDto(metaclass=InstanceCheckMeta):
    data_len = None
    fusion_op_names = None
    fusion_op_num = None
    level = None
    memory_input = None
    memory_output = None
    memory_total = None
    memory_weight = None
    memory_workspace = None
    op_name = None
    struct_type = None
    thread_id = None
    timestamp = None
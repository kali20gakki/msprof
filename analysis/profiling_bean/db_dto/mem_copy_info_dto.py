#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.


from dataclasses import dataclass
from profiling_bean.db_dto.dto_meta_class import InstanceCheckMeta


@dataclass
class MemCopyInfoDto(metaclass=InstanceCheckMeta):
    data_len = None
    data_size = None
    level = None
    memcpy_direction = None
    struct_type = None
    thread_id = None
    timestamp = None
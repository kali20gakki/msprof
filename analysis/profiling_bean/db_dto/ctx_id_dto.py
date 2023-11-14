#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.


from dataclasses import dataclass
from profiling_bean.db_dto.dto_meta_class import InstanceCheckMeta


@dataclass
class CtxIdDto(metaclass=InstanceCheckMeta):
    INVALID_CONTEXT_ID = "4294967295"
    ctx_id = INVALID_CONTEXT_ID
    ctx_id_num = None
    data_len = None
    level = None
    op_name = None
    struct_type = None
    thread_id = None
    timestamp = None
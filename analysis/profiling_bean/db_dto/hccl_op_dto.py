#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.


from dataclasses import dataclass
from profiling_bean.db_dto.dto_meta_class import InstanceCheckMeta


@dataclass
class HCCLOpDto(metaclass=InstanceCheckMeta):
    end: float = None
    index_id: int = None
    model_id: int = None
    op_name: str = None
    start: int = None
    thread_id: int = None

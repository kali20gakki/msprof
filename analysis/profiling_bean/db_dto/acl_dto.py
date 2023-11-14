#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.


from dataclasses import dataclass
from profiling_bean.db_dto.dto_meta_class import InstanceCheckMeta


@dataclass
class AclDto(metaclass=InstanceCheckMeta):
    """
    Dto for acl data
    """
    api_name = None
    api_type = None
    end_time = None
    start_time = None
    thread_id = None

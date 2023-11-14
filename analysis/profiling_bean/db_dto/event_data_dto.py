#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.


from dataclasses import dataclass
from profiling_bean.db_dto.dto_meta_class import InstanceCheckMeta


@dataclass
class EventDataDto(metaclass=InstanceCheckMeta):
    connection_id = None
    item_id = None
    level = None
    request_id = None
    struct_type = None
    thread_id = None
    timestamp = None

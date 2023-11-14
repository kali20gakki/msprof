#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.


from dataclasses import dataclass
from profiling_bean.db_dto.dto_meta_class import InstanceCheckMeta


@dataclass
class HostQueueDto(metaclass=InstanceCheckMeta):
    get_time = None
    index_id = None
    mode = None
    queue_capacity = None
    queue_size = None
    send_time = None
    total_time = None
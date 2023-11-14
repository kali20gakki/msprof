#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.


from dataclasses import dataclass
from profiling_bean.db_dto.dto_meta_class import InstanceCheckMeta


@dataclass
class DataQueueDto(metaclass=InstanceCheckMeta):
    duration = None
    end_time = None
    node_name = None
    queue_size = None
    start_time = None
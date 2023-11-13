#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
from dataclasses import dataclass
from profiling_bean.db_dto.dto_meta_class import InstanceCheckMeta


@dataclass
class AccPmuOriDto(metaclass=InstanceCheckMeta):
    """
    Dto for acc pmu data
    """
    acc_id = None
    block_id = None
    read_bandwidth = None
    read_ost = None
    stream_id = None
    task_id = None
    timestamp = None
    write_bandwidth = None
    write_ost = None

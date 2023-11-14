#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.


from dataclasses import dataclass
from profiling_bean.db_dto.dto_meta_class import InstanceCheckMeta


@dataclass
class CollectiveCommunicationDto(metaclass=InstanceCheckMeta):
    communication_time = None
    compute_time = None
    rank_id = None
    stage_time = None

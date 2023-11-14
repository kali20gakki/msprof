#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.

from dataclasses import dataclass
from common_func.constant import Constant
from profiling_bean.db_dto.dto_meta_class import InstanceCheckMeta


@dataclass
class TimeSectionDto(metaclass=InstanceCheckMeta):
    duration_time = -Constant.DEFAULT_INVALID_VALUE
    end_time = Constant.DEFAULT_INVALID_VALUE
    index_id = Constant.DEFAULT_INVALID_VALUE
    model_id = Constant.DEFAULT_INVALID_VALUE
    op_name = None
    overlap_time = Constant.DEFAULT_INVALID_VALUE
    start_time = Constant.DEFAULT_INVALID_VALUE
    stream_id = Constant.DEFAULT_INVALID_VALUE
    task_id = Constant.DEFAULT_INVALID_VALUE
    task_type = None


class CommunicationTimeSection(TimeSectionDto):
    def __init__(self):
        super().__init__()

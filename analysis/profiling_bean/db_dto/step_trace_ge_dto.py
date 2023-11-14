#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.

from dataclasses import dataclass
from common_func.constant import Constant
from profiling_bean.db_dto.dto_meta_class import InstanceCheckMeta


@dataclass
class StepTraceGeDto(metaclass=InstanceCheckMeta):
    index_id = Constant.DEFAULT_INVALID_VALUE
    model_id = Constant.DEFAULT_INVALID_VALUE
    op_name = Constant.NA
    op_type = Constant.NA
    stream_id = Constant.DEFAULT_INVALID_VALUE
    tag_id = Constant.DEFAULT_INVALID_VALUE
    task_id = Constant.DEFAULT_INVALID_VALUE
    timestamp = Constant.DEFAULT_INVALID_VALUE

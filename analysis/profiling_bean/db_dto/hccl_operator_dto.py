#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

from dataclasses import dataclass
from common_func.constant import Constant
from profiling_bean.db_dto.dto_meta_class import InstanceCheckMeta


@dataclass
class HCCLOperatorDto(metaclass=InstanceCheckMeta):
    end_time = Constant.DEFAULT_INVALID_VALUE
    index_id = Constant.DEFAULT_INVALID_VALUE
    model_id = Constant.DEFAULT_INVALID_VALUE
    op_name = Constant.NA
    op_type = Constant.NA
    overlap_time = Constant.DEFAULT_INVALID_VALUE
    start_time = Constant.DEFAULT_INVALID_VALUE
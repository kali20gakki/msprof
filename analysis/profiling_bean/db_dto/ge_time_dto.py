#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

from dataclasses import dataclass
from common_func.constant import Constant
from profiling_bean.db_dto.dto_meta_class import InstanceCheckMeta


@dataclass
class GeTimeDto(metaclass=InstanceCheckMeta):
    infer_end = Constant.DEFAULT_VALUE
    infer_start = Constant.DEFAULT_VALUE
    input_end = Constant.DEFAULT_VALUE
    input_start = Constant.DEFAULT_VALUE
    model_id = Constant.DEFAULT_VALUE
    model_name = None
    output_end = Constant.DEFAULT_VALUE
    output_start = Constant.DEFAULT_VALUE
    request_id = Constant.DEFAULT_VALUE
    stage_num = Constant.DEFAULT_VALUE
    thread_id = Constant.DEFAULT_VALUE

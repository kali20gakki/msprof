#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

from dataclasses import dataclass
from profiling_bean.db_dto.dto_meta_class import InstanceCheckMeta


@dataclass
class TensorInfoDto(metaclass=InstanceCheckMeta):
    data_len = None
    input_data_types = None
    input_formats = None
    input_shapes = None
    level = None
    op_name = None
    output_data_types = None
    output_formats = None
    output_shapes = None
    struct_type = None
    tensor_num = None
    thread_id = None
    timestamp = None

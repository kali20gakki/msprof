#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.

from dataclasses import dataclass

from common_func.constant import Constant
from profiling_bean.db_dto.dto_meta_class import InstanceCheckMeta


@dataclass
class RuntimeOpInfoDto(metaclass=InstanceCheckMeta):
    # device_id, stream_id, task_id仅作为key使用 默认值使用什么不影响业务
    is_valid: bool = False
    device_id: int = 0
    model_id: int = Constant.GE_OP_MODEL_ID
    stream_id: int = 0
    task_id: int = 0
    op_name: str = ""
    task_type: str = Constant.NA
    op_type: str = Constant.NA
    block_dim: int = 0
    mix_block_dim: int = 0
    op_flag: int = 0
    is_dynamic: str = Constant.NA
    tensor_num: int = None
    input_formats: str = None
    input_data_types: str = None
    input_shapes: str = None
    output_formats: str = None
    output_data_types: str = None
    output_shapes: str = None

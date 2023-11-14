#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
from dataclasses import dataclass
from profiling_bean.db_dto.dto_meta_class import InstanceCheckMeta


@dataclass
class TorchNpuDto(metaclass=InstanceCheckMeta):
    """
    Dto for relationship between torch op and npu kernel
    """
    acl_compile_time = None
    acl_end_time = None
    acl_start_time = None
    acl_tid = None
    batch_id = None
    context_id = None
    op_name = None
    stream_id = None
    task_id = None
    torch_op_pid = None
    torch_op_start_time = None
    torch_op_tid = None

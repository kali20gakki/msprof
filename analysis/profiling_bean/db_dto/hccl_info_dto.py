#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

from dataclasses import dataclass
from common_func.constant import Constant
from common_func.ms_constant.number_constant import NumberConstant
from profiling_bean.db_dto.dto_meta_class import InstanceCheckMeta


@dataclass
class HCCLInfoDto(metaclass=InstanceCheckMeta):
    ccl_tag = Constant.NA
    context_id = NumberConstant.DEFAULT_GE_CONTEXT_ID
    data_len = Constant.DEFAULT_INVALID_VALUE
    data_type = Constant.NA
    dst_addr = Constant.DEFAULT_INVALID_VALUE
    duration_estimated = Constant.DEFAULT_INVALID_VALUE
    group_name = Constant.NA
    level = Constant.NA
    link_type = Constant.NA
    local_rank = Constant.DEFAULT_INVALID_VALUE
    notify_id = Constant.DEFAULT_INVALID_VALUE
    op_name = Constant.NA
    op_type = Constant.NA
    plane_id = Constant.DEFAULT_INVALID_VALUE
    rank_size = Constant.DEFAULT_INVALID_VALUE
    rdma_type = Constant.NA
    remote_rank = Constant.DEFAULT_INVALID_VALUE
    role = Constant.NA
    size = Constant.DEFAULT_INVALID_VALUE
    src_addr = Constant.DEFAULT_INVALID_VALUE
    stage = Constant.DEFAULT_INVALID_VALUE
    struct_type = Constant.NA
    thread_id = Constant.DEFAULT_INVALID_VALUE
    timestamp = Constant.DEFAULT_INVALID_VALUE
    transport_type = Constant.NA
    work_flow_mode = Constant.NA

    def to_args_json(self, stream_id, task_id):
        json = {
            'notify_id': self.notify_id,
            'duration estimated(us)': self.duration_estimated,
            'stream id': stream_id,
            'task id': task_id,
            'context id': self.context_id,
            'task type': self.op_name,
            'src rank': self.local_rank,
            'dst rank': self.remote_rank,
            'transport type': self.transport_type,
            'size(Byte)': self.size,
            'data type': self.data_type,
            'link type': self.link_type,
        }
        return str(json)

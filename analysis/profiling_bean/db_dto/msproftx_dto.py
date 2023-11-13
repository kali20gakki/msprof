#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.

from dataclasses import dataclass
from common_func.constant import Constant
from profiling_bean.db_dto.dto_meta_class import InstanceCheckMeta


@dataclass
class MsprofTxDto(metaclass=InstanceCheckMeta):
    CHAR_LIST_SIZE = 128
    call_stack = Constant.NA
    category = 0
    dur_time = 0
    end_time = 0
    event_type = 0
    message = Constant.NA
    message_type = 0
    payload_type = 0
    payload_value = 0
    pid = 0
    start_time = 0
    tid = 0

    @property
    def is_enqueue_category(self: any) -> bool:
        return self.category == 0

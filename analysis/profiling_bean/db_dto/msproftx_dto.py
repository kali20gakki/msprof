#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.

from dataclasses import dataclass
from common_func.constant import Constant
from profiling_bean.db_dto.dto_meta_class import InstanceCheckMeta


@dataclass
class MsprofTxDto(metaclass=InstanceCheckMeta):
    call_stack: str = Constant.NA
    category: int = 0
    dur_time: int = 0
    end_time: int = 0
    event_type: int = 0
    message: str = Constant.NA
    message_type: int = 0
    payload_type: int = 0
    payload_value: int = 0
    pid: int = 0
    start_time: int = 0
    tid: int = 0

    @property
    def is_enqueue_category(self: any) -> bool:
        return self.category == 0

#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

from mscalculate.cann.additional_record import AdditionalRecord
from mscalculate.cann.event import Event
from common_func.constant import Constant
from common_func.singleton import singleton
from profiling_bean.db_dto.api_data_dto import ApiDataDto


class CANNDatabase:
    LEVELS_MAP = {
        "acl": Constant.ACL_LEVEL,
        "model": Constant.MODEL_LEVEL,
        "node": Constant.NODE_LEVEL,
        "hccl": Constant.HCCL_LEVEL,
        "runtime": Constant.TASK_LEVEL,
    }


@singleton
class ApiDataDatabase(CANNDatabase):
    def __init__(self):
        self._max_bound = -1
        self._data = dict()

    def put(self, data: ApiDataDto) -> Event:
        event = Event(
            self.LEVELS_MAP.get(data.level, data.level), data.thread_id, data.start, data.end, data.struct_type,
                                data.item_id)
        self._max_bound = max(data.end, self._max_bound)

        self._data[event] = data
        return event

    def get(self, event: Event) -> ApiDataDto:
        return self._data.get(event, ApiDataDto())

    def get_max_bound(self):
        return self._max_bound


@singleton
class AdditionalRecordDatabase(CANNDatabase):

    def __init__(self):
        self._data = dict()

    def put(self, data: AdditionalRecord) -> Event:
        dto = data.dto
        event = Event(self.LEVELS_MAP.get(dto.level), dto.thread_id, dto.timestamp, dto.timestamp, dto.struct_type)
        self._data[event] = data
        return event

    def get(self, event: Event) -> AdditionalRecord:
        return self._data.get(event, AdditionalRecord())

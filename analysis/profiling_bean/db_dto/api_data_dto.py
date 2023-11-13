#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

from dataclasses import dataclass
from profiling_bean.db_dto.event_data_dto import EventDataDto
from profiling_bean.db_dto.dto_meta_class import InstanceCheckMeta


@dataclass
class ApiDataDto(metaclass=InstanceCheckMeta):
    INVALID_LEVEL = -1
    INVALID_THREAD = -1
    connection_id = None
    end = None
    id = None
    item_id = None
    level = None
    request_id = None
    start = None
    struct_type = None
    thread_id = None

    def __init__(self: any, begin_time=None, end_event_data_dto: EventDataDto = EventDataDto()) -> None:
        self.struct_type = end_event_data_dto.struct_type
        self.id = None
        self.level = end_event_data_dto.level
        self.thread_id = end_event_data_dto.thread_id
        self.start = begin_time
        self.end = end_event_data_dto.timestamp
        self.item_id = end_event_data_dto.item_id
        self.request_id = end_event_data_dto.request_id
        self.connection_id = end_event_data_dto.connection_id

    @staticmethod
    def invalid_dto(level=INVALID_LEVEL, thread=INVALID_THREAD, start=-1, end=-1, struct_type=""):
        dto = ApiDataDto()
        dto.level = level
        dto.thread_id = thread
        dto.struct_type = struct_type
        dto.start = start
        dto.end = end
        dto.item_id = ""
        dto.connection_id = None
        return dto

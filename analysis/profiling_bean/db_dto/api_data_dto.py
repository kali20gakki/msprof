#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
from profiling_bean.db_dto.event_data_dto import EventDataDto


class ApiDataDto:
    INVALID_LEVEL = -1
    INVALID_THREAD = -1

    def __init__(self: any, begin_time=None, end_event_data_dto: EventDataDto = EventDataDto()) -> None:
        self.struct_type = end_event_data_dto.struct_type
        self._id = None
        self._level = end_event_data_dto.level
        self._thread_id = end_event_data_dto.thread_id
        self._start = begin_time
        self._end = end_event_data_dto.timestamp
        self._item_id = end_event_data_dto.item_id
        self._request_id = end_event_data_dto.request_id

    @property
    def struct_type(self: any) -> str:
        return self._struct_type

    @property
    def id(self: any) -> str:
        return str(self._id)

    @property
    def start(self: any) -> int:
        return self._start

    @property
    def end(self: any) -> int:
        return self._end

    @property
    def thread_id(self: any) -> int:
        return self._thread_id

    @property
    def item_id(self: any) -> int:
        return self._item_id

    @property
    def level(self: any) -> str:
        return self._level

    @property
    def request_id(self: any) -> int:
        return self._request_id

    @staticmethod
    def invalid_dto(level=INVALID_LEVEL, thread=INVALID_THREAD, start=-1, end=-1, struct_type=""):
        dto = ApiDataDto()
        dto.level = level
        dto.thread_id = thread
        dto.struct_type = struct_type
        dto.start = start
        dto.end = end
        return dto

    @struct_type.setter
    def struct_type(self: any, value: any) -> None:
        self._struct_type = value

    @id.setter
    def id(self: any, value: any) -> None:
        self._id = value

    @start.setter
    def start(self: any, value: any) -> None:
        self._start = value

    @end.setter
    def end(self: any, value: any) -> None:
        self._end = value

    @thread_id.setter
    def thread_id(self: any, value: any) -> None:
        self._thread_id = value

    @item_id.setter
    def item_id(self: any, value: any) -> None:
        self._item_id = value

    @level.setter
    def level(self: any, value: any) -> None:
        self._level = value

    @request_id.setter
    def request_id(self: any, value: any) -> None:
        self._request_id = value

#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

class NpuOpMemDto:
    """
    Dto for npu op mem data
    """
    def __init__(self: any) -> None:
        self._operator = None
        self._addr = None
        self._size = None
        self._timestamp = None
        self._thread_id = None

        self._total_allocate_memory = None
        self._total_reserve_memory = None

        self._level = None
        self._type = None

        self._device_type = None

    @property
    def operator(self: any) -> any:
        return self._operator

    @property
    def addr(self: any) -> any:
        return self._addr

    @property
    def size(self: any) -> any:
        return self._size

    @property
    def timestamp(self: any) -> any:
        return self._timestamp

    @property
    def thread_id(self: any) -> any:
        return self._thread_id

    @property
    def total_allocate_memory(self: any) -> any:
        return self._total_allocate_memory

    @property
    def total_reserve_memory(self: any) -> any:
        return self._total_reserve_memory

    @property
    def level(self: any) -> any:
        return self._level

    @property
    def type(self: any) -> any:
        return self._type

    @property
    def device_type(self: any) -> any:
        return self._device_type

    @operator.setter
    def operator(self: any, value: any) -> None:
        self._operator = value

    @addr.setter
    def addr(self: any, value: any) -> None:
        self._addr = value

    @size.setter
    def size(self: any, value: any) -> None:
        self._size = value

    @timestamp.setter
    def timestamp(self: any, value: any) -> None:
        self._timestamp = value

    @thread_id.setter
    def thread_id(self: any, value: any) -> None:
        self._thread_id = value

    @total_allocate_memory.setter
    def total_allocate_memory(self: any, value: any) -> None:
        self._total_allocate_memory = value

    @total_reserve_memory.setter
    def total_reserve_memory(self: any, value: any) -> None:
        self._total_reserve_memory = value

    @level.setter
    def level(self: any, value: any) -> None:
        self._level = value

    @type.setter
    def type(self: any, value: any) -> None:
        self._type = value

    @device_type.setter
    def device_type(self: any, value: any) -> None:
        self._device_type = value
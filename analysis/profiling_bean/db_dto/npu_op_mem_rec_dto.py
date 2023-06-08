#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

class NpuOpMemRecDto:
    """
    Dto for npu op mem data
    """
    def __init__(self: any) -> None:
        self._component = None
        self._timestamp = None
        self._total_reserve_memory = None
        self._total_allocate_memory = None
        self._device_type = None

    @property
    def component(self: any) -> any:
        return self._component

    @property
    def timestamp(self: any) -> any:
        return self._timestamp

    @property
    def total_reserve_memory(self: any) -> any:
        return self._total_reserve_memory

    @property
    def total_allocate_memory(self: any) -> any:
        return self._total_allocate_memory

    @property
    def device_type(self: any) -> any:
        return self._device_type

    @component.setter
    def component(self: any, value: any) -> None:
        self._component = value

    @timestamp.setter
    def timestamp(self: any, value: any) -> None:
        self._timestamp = value

    @total_allocate_memory.setter
    def total_allocate_memory(self: any, value: any) -> None:
        self._total_allocate_memory = value

    @total_reserve_memory.setter
    def total_reserve_memory(self: any, value: any) -> None:
        self._total_reserve_memory = value

    @device_type.setter
    def device_type(self: any, value: any) -> None:
        self._device_type = value
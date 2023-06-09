#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

class OpMemDto:
    """
    Dto for npu op mem data
    """
    def __init__(self: any) -> None:
        self._operator = None
        self._size = None
        self._allocation_time = None
        self._release_time = None

        self._duration = None

        self._allocation_total_allocated = None
        self._allocation_total_reserved = None
        self._release_total_allocated = None
        self._release_total_reserved = None

        self._device_type = None
        self._name = None

    @property
    def name(self: any) -> any:
        return self._name

    @property
    def operator(self: any) -> any:
        return self._operator

    @property
    def size(self: any) -> any:
        return self._size

    @property
    def allocation_time(self: any) -> any:
        return self._allocation_time

    @property
    def release_time(self: any) -> any:
        return self._release_time

    @property
    def duration(self: any) -> any:
        return self._duration

    @property
    def allocation_total_allocated(self: any) -> any:
        return self._allocation_total_allocated

    @property
    def allocation_total_reserved(self: any) -> any:
        return self._allocation_total_reserved

    @property
    def release_total_allocated(self: any) -> any:
        return self._release_total_allocated

    @property
    def release_total_reserved(self: any) -> any:
        return self._release_total_reserved

    @property
    def device_type(self: any) -> any:
        return self._device_type

    @name.setter
    def name(self: any, value: any) -> None:
        self._name = value

    @operator.setter
    def operator(self: any, value: any) -> None:
        self._operator = value

    @size.setter
    def size(self: any, value: any) -> None:
        self._size = value

    @allocation_time.setter
    def allocation_time(self: any, value: any) -> None:
        self._allocation_time = value

    @release_time.setter
    def release_time(self: any, value: any) -> None:
        self._release_time = value

    @duration.setter
    def duration(self: any, value: any) -> None:
        self._duration = value

    @allocation_total_allocated.setter
    def allocation_total_allocated(self: any, value: any) -> None:
        self._allocation_total_allocated = value

    @allocation_total_reserved.setter
    def allocation_total_reserved(self: any, value: any) -> None:
        self._allocation_total_reserved = value

    @release_total_allocated.setter
    def release_total_allocated(self: any, value: any) -> None:
        self._release_total_allocated = value

    @release_total_reserved.setter
    def release_total_reserved(self: any, value: any) -> None:
        self._release_total_reserved = value

    @device_type.setter
    def device_type(self: any, value: any) -> None:
        self._device_type = value
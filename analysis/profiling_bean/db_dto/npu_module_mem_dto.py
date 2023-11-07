#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.


class NpuModuleMemDto:
    """
    Dto for npu module mem data
    """

    def __init__(self: any) -> None:
        self._module_id = None
        self._syscnt = None
        self._total_size = None
        self._device_type = None

    @property
    def module_id(self: any) -> any:
        return self._module_id

    @property
    def syscnt(self: any) -> any:
        return self._syscnt

    @property
    def total_size(self: any) -> any:
        return self._total_size

    @property
    def device_type(self: any) -> any:
        return self._device_type

    @module_id.setter
    def module_id(self: any, value: any) -> None:
        self._module_id = value

    @syscnt.setter
    def syscnt(self: any, value: any) -> None:
        self._syscnt = value

    @total_size.setter
    def total_size(self: any, value: any) -> None:
        self._total_size = value

    @device_type.setter
    def device_type(self: any, value: any) -> None:
        self._device_type = value

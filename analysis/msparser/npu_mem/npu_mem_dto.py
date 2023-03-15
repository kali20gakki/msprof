#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

class NpuMemDto:
    """
    Dto for npu mem data
    """

    def __init__(self: any) -> None:
        self._event = None
        self._ddr = None
        self._hbm = None
        self._memory = None
        self._timestamp = None

    @property
    def event(self: any) -> any:
        return self._event

    @property
    def ddr(self: any) -> any:
        return self._ddr

    @property
    def hbm(self: any) -> any:
        return self._hbm

    @property
    def memory(self: any) -> any:
        return self._memory

    @property
    def timestamp(self: any) -> any:
        return self._timestamp

    @event.setter
    def event(self: any, value: any) -> None:
        self._event = value

    @ddr.setter
    def ddr(self: any, value: any) -> None:
        self._ddr = value

    @hbm.setter
    def hbm(self: any, value: any) -> None:
        self._hbm = value

    @memory.setter
    def memory(self: any, value: any) -> None:
        self._memory = value

    @timestamp.setter
    def timestamp(self: any, value: any) -> None:
        self._timestamp = value

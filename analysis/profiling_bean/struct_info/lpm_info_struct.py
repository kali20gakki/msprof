#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.


class LpmData:
    """
    lpm data struct
    """

    def __init__(self: any, syscnt: int, value: int) -> None:
        self._syscnt = syscnt
        self._value = value

    @property
    def syscnt(self: any) -> int:
        """
        cycle
        """
        return self._syscnt

    @property
    def value(self: any) -> int:
        """
        TYPE_AIC_FREQ: frequency, MHz
        TYPE_AIC_VOLTAGE: voltage, mV
        TYPE_BUS_VOLTAGE: voltage, mV
        """
        return self._value

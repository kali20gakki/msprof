#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.


class FopsDto:
    """
    Dto for fops data
    """

    def __init__(self: any) -> None:
        self._cube_fops = None
        self._vector_fops = None
        self._total_fops = None
        self._op_type = None
        self._total_time = None

    @property
    def cube_fops(self: any) -> any:
        return self._cube_fops

    @property
    def total_fops(self: any) -> any:
        return self._total_fops

    @property
    def op_type(self: any) -> any:
        return self._op_type

    @property
    def total_time(self: any) -> any:
        return self._total_time

    @cube_fops.setter
    def cube_fops(self: any, value: any) -> None:
        self._cube_fops = value

    @total_fops.setter
    def total_fops(self: any, value: any) -> None:
        self._total_fops = value

    @op_type.setter
    def op_type(self: any, value: any) -> None:
        self._op_type = value

    @total_time.setter
    def total_time(self: any, value: any) -> None:
        self._total_time = value

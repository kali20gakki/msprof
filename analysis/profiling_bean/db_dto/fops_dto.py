#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2022. All rights reserved.
"""


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

    @cube_fops.setter
    def cube_fops(self: any, value: any) -> None:
        self._cube_fops = value

    @property
    def total_fops(self: any) -> any:
        return self._total_fops

    @total_fops.setter
    def total_fops(self: any, value: any) -> None:
        self._total_fops = value

    @property
    def op_type(self: any) -> any:
        return self._op_type

    @op_type.setter
    def op_type(self: any, value: any) -> None:
        self._op_type = value

    @property
    def total_time(self: any) -> any:
        return self._total_time

    @total_time.setter
    def total_time(self: any, value: any) -> None:
        self._total_time = value


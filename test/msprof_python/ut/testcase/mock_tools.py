#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2022. All rights reserved.
"""
from unittest import mock


class ClassMock:
    def __init__(self: any, owner: any, instance: any) -> None:
        self.owner = owner
        self.instance = instance

    def __enter__(self: any) -> None:
        self.owner.__new__ = self.instance

    def __exit__(self: any, exc_type, exc_val, exc_tb) -> None:
        self.owner.__new__ = ClassMock.origin_new_function

    @staticmethod
    def origin_new_function(cls: any, *args, **kwargs) -> any:
        return object().__new__(cls)

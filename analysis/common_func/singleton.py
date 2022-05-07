#!/usr/bin/python3
# coding=utf-8
"""
Function: This file mainly involves the os function.
Copyright Information:
Huawei Technologies Co., Ltd. All Rights Reserved © 2020
"""


def singleton(cls: any) -> any:
    """
    singleton Decorators
    """
    _instance = {}

    def _singleton(*args: any, **kw: any) -> any:
        if cls not in _instance:
            _instance[cls] = cls(*args, **kw)
        return _instance.get(cls)

    return _singleton

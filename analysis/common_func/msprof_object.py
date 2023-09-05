#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

"""
This file used for basic python type modification
"""
from typing import OrderedDict


class HighPerfDict(OrderedDict):
    """
    this class will provide some high perf interface for dict
    """
    def set_default_call_obj_later(self: any, key: any, _class: any, *args, **kwargs):
        if key in self.keys():
            return self.get(key)
        obj = _class(*args, **kwargs)
        self.setdefault(key, obj)
        return obj

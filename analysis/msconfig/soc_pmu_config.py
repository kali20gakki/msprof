#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

from msconfig.meta_config import MetaConfig


class SocPmuConfig(MetaConfig):
    DEFAULT_EVENT = [
        ('request_events', '0x8a'),
        ('hit_events', '0x8c,0x8d'),
        ('miss_events', '0x2')
    ]

    DATA = {
        '5': DEFAULT_EVENT,
        '15': DEFAULT_EVENT,
        '16': DEFAULT_EVENT,
    }

    def __init__(self):
        super().__init__()


#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

from config.meta_config import MetaConfig


class L2CacheConfig(MetaConfig):
    DATA = {
        '1': [
            ('request_events', '0x59'),
            ('hit_events', '0x5b'),
            ('victim_events', '0x5c')
        ],
        '2': [
            ('request_events', '0x78,0x79'),
            ('hit_events', '0x6a'),
            ('victim_events', '0x71')
        ],
        '3': [
            ('request_events', '0x78,0x79'),
            ('hit_events', '0x6a'),
            ('victim_events', '0x71')
        ],
        '4': [
            ('request_events', '0x78,0x79'),
            ('hit_events', '0x6a'),
            ('victim_events', '0x71')
        ]
    }

    def __init__(self):
        super().__init__()


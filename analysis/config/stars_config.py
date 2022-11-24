#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

from config.meta_config import MetaConfig


class StarsConfig(MetaConfig):
    DATA = {
        'AcsqTaskParser': [
            ('fmt', '000000, 000001'),
            ('db', 'acsq.db'),
            ('table', 'AcsqTask'),
            ('db_status', '')
        ]
        , 'FftsLogParser': [
            ('fmt', '100010, 100011'),
            ('db', 'soc_log.db'),
            ('table', 'FftsLog'),
            ('db_status', '')
        ]
        , 'AccPmuParser': [
            ('fmt', '011010'),
            ('db', 'acc_pmu.db'),
            ('table', 'AccPmuOrigin'),
            ('db_status', '1')
        ]
        , 'InterSocParser': [
            ('fmt', '011100'),
            ('db', 'soc_profiler.db'),
            ('table', 'InterSoc'),
            ('db_status', '1')
        ]
        , 'StarsChipTransParser': [
            ('fmt', '011011'),
            ('db', 'chip_trans.db'),
            ('table', 'PaLinkInfo,PcieInfo'),
            ('db_status', '1')
        ]
        , 'LowPowerParser': [
            ('fmt', '011101'),
            ('db', 'lowpower.db'),
            ('table', 'LowPower'),
            ('db_status', '1')
        ]
    }

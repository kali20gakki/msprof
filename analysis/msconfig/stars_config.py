#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

from msconfig.meta_config import MetaConfig


class StarsConfig(MetaConfig):
    DATA = {
        'AcsqTaskParser': [
            ('fmt', '000000, 000001'),
            ('db', 'acsq.db'),
            ('table', 'AcsqTask'),
            ('is_db_needed_clear', '1')
        ]
        , 'FftsLogParser': [
            ('fmt', '100010, 100011'),
            ('db', 'soc_log.db'),
            ('table', 'FftsLog'),
            ('is_db_needed_clear', '1')
        ]
        , 'AccPmuParser': [
            ('fmt', '011010'),
            ('db', 'acc_pmu.db'),
            ('table', 'AccPmu')
        ]
        , 'InterSocParser': [
            ('fmt', '011100'),
            ('db', 'soc_profiler.db'),
            ('table', 'InterSoc')
        ]
        , 'StarsChipTransParser': [
            ('fmt', '011011'),
            ('db', 'chip_trans.db'),
            ('table', 'PaLinkInfo,PcieInfo')
        ]
        , 'LowPowerParser': [
            ('fmt', '011101'),
            ('db', 'lowpower.db'),
            ('table', 'LowPower')
        ]
    }

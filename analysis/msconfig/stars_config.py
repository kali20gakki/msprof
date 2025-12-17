# -------------------------------------------------------------------------
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
# This file is part of the MindStudio project.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#    http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------

from msconfig.meta_config import MetaConfig


class StarsConfig(MetaConfig):
    DATA = {
        'AcsqTaskParser': [
            ('fmt', '000000, 000001'),
            ('db', 'soc_log.db'),
            ('table', 'AcsqTask')
        ],
        'FftsLogParser': [
            ('fmt', '100010, 100011'),
            ('db', 'soc_log.db'),
            ('table', 'FftsLog')
        ],
        'SioParser': [
            ('fmt', '011001'),
            ('db', 'sio.db'),
            ('table', 'Sio')
        ],
        'AccPmuParser': [
            ('fmt', '011010'),
            ('db', 'acc_pmu.db'),
            ('table', 'AccPmu')
        ],
        'InterSocParser': [
            ('fmt', '011100'),
            ('db', 'soc_profiler.db'),
            ('table', 'InterSoc')
        ],
        'StarsChipTransParser': [
            ('fmt', '011011'),
            ('db', 'chip_trans.db'),
            ('table', 'PaLinkInfo,PcieInfo')
        ],
        'LowPowerParser': [
            ('fmt', '011101'),
            ('db', 'lowpower.db'),
            ('table', 'LowPower')
        ]
    }

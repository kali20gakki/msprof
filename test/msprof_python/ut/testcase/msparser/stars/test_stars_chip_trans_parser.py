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
import struct
import unittest

from msparser.stars import StarsChipTransParser
from profiling_bean.stars.stars_chip_trans_bean import StarsChipTransBean

NAMESPACE = 'msparser.stars.stars_chip_trans_bean'
sample_config = {"model_id": 1, 'iter_id': 'dasfsd', 'result_dir': 'jasdfjfjs',
                 "ai_core_profiling_mode": "task-based", "aiv_profiling_mode": "sample-based"}


class TestStarsChipTransParser(unittest.TestCase):

    def test_decoder(self):
        check = StarsChipTransParser(sample_config.get('result_dir'), 'test.db', [])
        check._decoder = StarsChipTransBean(struct.pack("2HLQ2H3L2Q4L", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0))
        res = check.decoder
        self.assertEqual(res.sys_cnt, 0)

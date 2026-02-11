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

from msparser.stars.fusion_task_parser import FusionTaskParser
from common_func.platform.chip_manager import ChipManager
from profiling_bean.prof_enum.chip_model import ChipModel
from common_func.info_conf_reader import InfoConfReader

NAMESPACE = 'msparser.stars.fusion_task_parser'
sample_config = {"model_id": 1, 'iter_id': '1919', 'result_dir': '114514',
                 "ai_core_profiling_mode": "task-based", "aiv_profiling_mode": "sample-based"}

class FusionTaskParserTest(unittest.TestCase):

    def test_fusion_task_parser(self):
        check = FusionTaskParser(sample_config.get('result_dir'), 'test.db', [])
        check.handle("010111", "114514")
        check.preprocess_data()
        check.flush()

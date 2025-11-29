#!/usr/bin/python3
# -*- coding: utf-8 -*-
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
import os
import struct
import shutil
import unittest
from unittest import mock

from msparser.data_struct_size_constant import StructFmt
from msparser.add_info.mc2_comm_info_bean import Mc2CommInfoBean
from msparser.add_info.mc2_comm_info_parser import Mc2CommInfoParser
from profiling_bean.db_dto.step_trace_dto import IterationRange
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.add_info.mc2_comm_info_parser'


class TestMc2CommInfoParser(unittest.TestCase):
    file_list = {
        DataTag.MC2_COMM_INFO: [
            'aging.additional.mc2_comm_info.slice_0'
        ]
    }
    DIR_PATH = os.path.join(os.path.dirname(__file__), "mc2_comm_info")
    SQLITE_PATH = os.path.join(DIR_PATH, "sqlite")
    CONFIG = {
        'result_dir': DIR_PATH, 'device_id': '0', 'iter_id': IterationRange(0, 1, 1),
        'job_id': 'job_default', 'model_id': -1
    }

    def setup_class(self):
        if not os.path.exists(self.DIR_PATH):
            os.mkdir(self.DIR_PATH)
        if not os.path.exists(self.SQLITE_PATH):
            os.mkdir(self.SQLITE_PATH)

    def teardown_class(self):
        if os.path.exists(self.DIR_PATH):
            shutil.rmtree(self.DIR_PATH)

    def test_parse_should_return_1_mc2_comm_info_data_when_256_bytes_in_file(self):
        mc2_comm_info_data = [
            23130, 10000, 0, 1, 128, 2000,
            7466789422691968299, 8, 0, 0, 1, 8, 52, 53, 54, 55, 56, 57, 58, 59
        ] + [0] * 43
        struct_data = struct.pack(StructFmt.MC2_COMM_INFO_FMT, *mc2_comm_info_data)
        data = Mc2CommInfoBean.decode(struct_data)
        with mock.patch(NAMESPACE + '.Mc2CommInfoParser.parse_bean_data', return_value=[data]):
            check = Mc2CommInfoParser(self.file_list, self.CONFIG)
            check.parse()
            check.save()
        self.assertEqual(1, len(check._communication_info))
        self.assertEqual("7466789422691968299", check._communication_info[0].group_name)

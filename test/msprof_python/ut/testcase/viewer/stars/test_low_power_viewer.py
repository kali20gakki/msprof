#!/usr/bin/env python
# coding=utf-8
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
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import unittest
from common_func.info_conf_reader import InfoConfReader
from viewer.stars.low_power_viewer import LowPowerViewer


class TestLowPowerViewer(unittest.TestCase):

    def test_get_trace_timeline_should_return_timeline_format_when_only_ddie_data_exists(self):
        InfoConfReader()._info_json = {"pid": '0'}
        low_power_viewer = LowPowerViewer({}, {'data_type': "low_power"})
        data = [
            (69869729100545, 0, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20),
            (69869749580545, 1, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20),
        ]
        ret = low_power_viewer.get_trace_timeline(data)
        self.assertEqual(29, len(ret))

    def test_get_trace_timeline_should_return_timeline_format_when_only_udie_data_exists(self):
        InfoConfReader()._info_json = {"pid": '0'}
        low_power_viewer = LowPowerViewer({}, {'data_type': "low_power"})
        data = [
            (69869729100545, 2, 10, 10, 10, 10, 10, 10, 10, 10, 10, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20),
            (69869749580545, 3, 10, 10, 10, 10, 10, 10, 10, 10, 10, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20),
        ]
        ret = low_power_viewer.get_trace_timeline(data)
        self.assertEqual(19, len(ret))

    def test_get_trace_timeline_should_return_timeline_format_when_ddie_udie_data_exists(self):
        InfoConfReader()._info_json = {"pid": '0'}
        low_power_viewer = LowPowerViewer({}, {'data_type': "low_power"})
        data = [
            (69869729100545, 1, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20),
            (69869749580545, 3, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20)
        ]
        ret = low_power_viewer.get_trace_timeline(data)
        self.assertEqual(24, len(ret))

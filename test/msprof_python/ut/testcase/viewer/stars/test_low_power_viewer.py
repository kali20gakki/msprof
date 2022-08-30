#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import unittest

from viewer.stars.low_power_viewer import LowPowerViewer

from common_func.info_conf_reader import InfoConfReader

NAMESPACE = 'viewer.stars.low_power_view'


class TestLowPowerViewer(unittest.TestCase):

    def setup_class(self) -> None:
        self.low_power_viewer = LowPowerViewer({}, {'data_type': "low_power"})

    def test_get_timeline_header(self):
        InfoConfReader()._info_json = {"pid": "0"}
        ret = self.low_power_viewer.get_timeline_header()
        self.assertEqual(1, len(ret))

    def test_get_trace_timeline(self):
        ret = self.low_power_viewer.get_trace_timeline([])
        self.assertEqual(0, len(ret))

        data = [[1, 2, 3, 4, 5, 6,
                 1, 2, 3, 4, 5, 6,
                 1, 2, 3, 4, 5, 6,
                 1, 2, 3, 4, 5, 6,
                 1, 2, 3, 4, 5, 6],
                [6, 5, 4, 3, 2, 1,
                 6, 5, 4, 3, 2, 1,
                 6, 5, 4, 3, 2, 1,
                 6, 5, 4, 3, 2, 1,
                 6, 5, 4, 3, 2, 1]
                ]
        ret = self.low_power_viewer.get_trace_timeline(data)
        self.assertEqual(37, len(ret))

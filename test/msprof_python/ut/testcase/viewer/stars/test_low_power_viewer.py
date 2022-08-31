#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import unittest

from constant.info_json_construct import InfoJson
from constant.info_json_construct import InfoJsonReaderManager
from viewer.stars.low_power_viewer import LowPowerViewer

NAMESPACE = 'viewer.stars.low_power_view'


class TestLowPowerViewer(unittest.TestCase):

    def test_get_timeline_header(self):
        InfoJsonReaderManager(InfoJson(pid=0)).process()
        low_power_viewer = LowPowerViewer({}, {'data_type': "low_power"})
        ret = low_power_viewer.get_timeline_header()
        self.assertEqual(1, len(ret))

    def test_get_trace_timeline(self):
        low_power_viewer = LowPowerViewer({}, {'data_type': "low_power"})
        ret = low_power_viewer.get_trace_timeline([])
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
        ret = low_power_viewer.get_trace_timeline(data)
        self.assertEqual(37, len(ret))

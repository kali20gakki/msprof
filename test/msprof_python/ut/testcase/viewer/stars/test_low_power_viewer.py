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

        data = [(
            69869729100545, 799, 799, 799, 10341.885481852316, 9885.874843554444, 106.38329161451814, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 5438.734981226533, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0),
            (69869749580545, 203, 203, 203, 43.064655172413794, 42.972290640394085, 56, 0, 0, 0, 0, 0, 0, 0, 0, 0,
             1500, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0)
        ]
        ret = low_power_viewer.get_trace_timeline(data)
        self.assertEqual(41, len(ret))

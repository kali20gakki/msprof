#!/usr/bin/env python
# coding=utf-8
"""
function: test mspti monitor
Copyright Huawei Technologies Co., Ltd. 2020-2024. All rights reserved.
"""

import unittest
from unittest import mock


class MsptiCMock:
    call_back = None

    @classmethod
    def start(cls):
        return 0

    @classmethod
    def stop(cls):
        return 0

    @classmethod
    def flush_all(cls):
        callable(cls.call_back) and cls.call_back({"deviceId": 4, "streamId": 2})
        return 0

    @classmethod
    def flush_period(cls, time_ms):
        return 0

    @classmethod
    def set_buffer_size(cls, size: int):
        return 0

    @classmethod
    def registerCB(cls, cb):
        cls.call_back = cb
        return 0

    @classmethod
    def unregisterCB(cls):
        cls.call_back = None
        return 0

    @classmethod
    def enableDomain(cls, domain_name):
        return 0

    @classmethod
    def disableDomain(cls, domain_name):
        return 0


class TestMsptiMonitor(unittest.TestCase):

    def test_kernel_monitor(self):
        with mock.patch('importlib.import_module', return_value=MsptiCMock):
            from mspti import utils, KernelMonitor, MsptiResult

            monitor = KernelMonitor()
            self.assertEqual(MsptiResult.MSPTI_SUCCESS, monitor.set_buffer_size(4))
            self.assertEqual(MsptiResult.MSPTI_SUCCESS, monitor.start(print))
            self.assertEqual(MsptiResult.MSPTI_SUCCESS, monitor.flush_period(1024))
            self.assertEqual(MsptiResult.MSPTI_SUCCESS, monitor.stop())
            self.assertEqual(MsptiResult.MSPTI_SUCCESS, monitor.flush_all())

    def test_mstx_monitor(self):
        with mock.patch('importlib.import_module', return_value=MsptiCMock):
            from mspti import utils, MstxMonitor, MsptiResult

            monitor = MstxMonitor()
            self.assertEqual(MsptiResult.MSPTI_SUCCESS, monitor.set_buffer_size(4))
            self.assertEqual(MsptiResult.MSPTI_SUCCESS, monitor.start(print))
            self.assertEqual(MsptiResult.MSPTI_SUCCESS, monitor.flush_period(1024))
            self.assertEqual(MsptiResult.MSPTI_SUCCESS, monitor.stop())
            self.assertEqual(MsptiResult.MSPTI_SUCCESS, monitor.flush_all())

    def test_range_mstx_monitor(self):
        with mock.patch('importlib.import_module', return_value=MsptiCMock):
            from mspti import utils, MstxMonitor, MsptiResult, RangeMarkerData

            def check_range_data(range_data: RangeMarkerData):
                self.assertEqual(50, range_data.start)
                self.assertEqual(100, range_data.end)

            monitor = MstxMonitor()
            self.assertEqual(MsptiResult.MSPTI_SUCCESS, monitor.start(range_cb=check_range_data))
            origin_data1 = {"id": 1, "flag": 1 << 2, "timestamp": 100}
            origin_data2 = {"id": 1, "flag": 1 << 1, "timestamp": 50}
            monitor.callback(origin_data1)
            monitor.callback(origin_data2)



if __name__ == '__main__':
    unittest.main()

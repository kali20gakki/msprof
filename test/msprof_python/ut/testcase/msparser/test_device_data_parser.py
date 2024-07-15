#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
import unittest
from unittest import mock
from msparser.device_data_parser import DeviceDataParser

NAMESPACE = 'common_func.config_mgr'


class TestDeviceDataParser(unittest.TestCase):
    file_list = {}
    config = {"result_dir": "/tmp/prof"}

    def test_ms_run_should_return_when_so_is_not_available(self):
        with mock.patch(NAMESPACE + '.ConfigMgr.is_ai_core_sample_based', return_value=False):
            check = DeviceDataParser(self.file_list, self.config)
            check.ms_run()

    def test_ms_run_should_return_when_pmu_is_sample_based(self):
        with mock.patch(NAMESPACE + '.ConfigMgr.is_ai_core_sample_based', return_value=True):
            check = DeviceDataParser(self.file_list, self.config)
            check.ms_run()


if __name__ == '__main__':
    unittest.main()

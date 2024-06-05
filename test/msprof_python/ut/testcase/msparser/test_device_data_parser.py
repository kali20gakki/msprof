#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
import unittest
from msparser.device_data_parser import DeviceDataParser


class TestDeviceDataParser(unittest.TestCase):
    file_list = {}
    config = {"result_dir": "/tmp/prof"}

    def test_ms_run_should_return_when_so_is_not_available(self):
        check = DeviceDataParser(self.file_list, self.config)
        check.ms_run()


if __name__ == '__main__':
    unittest.main()

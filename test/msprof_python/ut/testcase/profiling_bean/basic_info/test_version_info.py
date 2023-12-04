#!/usr/bin/python3
# -*- coding: utf-8 -*-
#  Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

import unittest

from constant.info_json_construct import InfoJsonReaderManager, InfoJson
from profiling_bean.basic_info.version_info import VersionInfo


class TestVersionInfo(unittest.TestCase):
    def test_run_when_driver_is_old_then_return_zero_for_driver_version(self):
        InfoJsonReaderManager(info_json=InfoJson(version='1.0')).process()
        checker = VersionInfo()
        checker.run("")
        self.assertEqual(0, checker.drv_version)

    def test_run_when_driver_update_then_return_acltual_driver_version(self):
        InfoJsonReaderManager(info_json=InfoJson(drvVersion=467731)).process()
        checker = VersionInfo()
        checker.run("")
        self.assertEqual(467731, checker.drv_version)


if __name__ == '__main__':
    unittest.main()

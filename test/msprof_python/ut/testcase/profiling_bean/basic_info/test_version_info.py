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

import unittest

from constant.info_json_construct import InfoJsonReaderManager, InfoJson
from profiling_bean.basic_info.version_info import VersionInfo


class TestVersionInfo(unittest.TestCase):
    def test_run_when_driver_is_old_then_return_zero_for_driver_version(self):
        InfoJsonReaderManager(info_json=InfoJson(version='1.0')).process()
        checker = VersionInfo()
        checker.run("")
        self.assertEqual(0, checker.drv_version)

    def test_run_when_driver_update_then_return_actual_driver_version(self):
        InfoJsonReaderManager(info_json=InfoJson(drvVersion=467731)).process()
        checker = VersionInfo()
        checker.run("")
        self.assertEqual(467731, checker.drv_version)


if __name__ == '__main__':
    unittest.main()

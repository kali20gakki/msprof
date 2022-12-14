#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import unittest

from msconfig.config_manager import ConfigManager
from framework.config_data_parsers import ConfigDataParsers
from profiling_bean.prof_enum.chip_model import ChipModel


class TestConfigDataParsers(unittest.TestCase):
    def test_get_parsers(self):
        ConfigDataParsers.get_parsers(ConfigManager.DATA_CALCULATOR, str(ChipModel.CHIP_V3_1_0.value))

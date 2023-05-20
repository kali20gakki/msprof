#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import unittest

from common_func.info_conf_reader import InfoConfReader
from msconfig.config_manager import ConfigManager
from framework.config_data_parsers import ConfigDataParsers
from profiling_bean.prof_enum.chip_model import ChipModel


class TestConfigDataParsers(unittest.TestCase):
    def test_get_parsers(self):
        InfoConfReader()._sample_json = {'devices': '0'}
        ConfigDataParsers.get_parsers(ConfigManager.DATA_CALCULATOR, str(ChipModel.CHIP_V3_1_0.value))

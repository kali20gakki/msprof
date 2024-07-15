#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import unittest

from common_func.info_conf_reader import InfoConfReader
from common_func.platform.chip_manager import ChipManager
from framework.config_data_parsers import ConfigDataParsers
from msconfig.config_manager import ConfigManager
from profiling_bean.prof_enum.chip_model import ChipModel


class TestConfigDataParsers(unittest.TestCase):
    def test_get_parsers(self):
        InfoConfReader()._sample_json = {'devices': '0'}
        ConfigDataParsers.get_parsers(ConfigManager.DATA_CALCULATOR, str(ChipModel.CHIP_V3_1_0.value), False)

    def test_load_can_cpp_parse(self):
        ret = ConfigDataParsers._load_can_cpp_parse("NpuMemParser")
        self.assertFalse(ret)
        ret = ConfigDataParsers._load_can_cpp_parse("HashDicParser")
        self.assertTrue(ret)

    def test_load_can_cpp_parse_or_calculate_device_data_should_return_true_when_given_in_whitelist(self):
        ChipManager().chip_id = ChipModel.CHIP_V4_1_0
        ret = ConfigDataParsers._load_can_cpp_parse_or_calculate_device_data("AscendTaskCalculator")
        self.assertTrue(ret)

    def test_load_can_cpp_parse_or_calculate_device_data_should_return_true_when_given_not_in_whitelist(self):
        ChipManager().chip_id = ChipModel.CHIP_V4_1_0
        ret = ConfigDataParsers._load_can_cpp_parse_or_calculate_device_data("NpuMemParser")
        self.assertFalse(ret)

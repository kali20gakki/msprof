#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import os
import unittest

from common_func.msvp_constant import MsvpConstant
from framework.config_data_parsers import ConfigDataParsers
from profiling_bean.prof_enum.chip_model import ChipModel

CONF_FILE = os.path.join(MsvpConstant.CONFIG_PATH, 'data_parsers.ini')


class TestConfigDataParsers(unittest.TestCase):
    def test_get_parsers(self):
        ConfigDataParsers.get_parsers(str(ChipModel.CHIP_V3_1_0.value), CONF_FILE)

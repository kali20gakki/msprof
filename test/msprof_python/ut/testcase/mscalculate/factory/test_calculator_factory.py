#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import unittest
from unittest import mock

from constant.constant import CONFIG
from mscalculate.factory.calculator_factory import CalculatorFactory

NAMESPACE = 'mscalculate.factory.calculator_factory'
CONFIG_NAMESPACE = 'common_func.config_mgr'


class TestCalculatorFactory(unittest.TestCase):
    def test_run(self):
        with mock.patch(NAMESPACE + '.ConfigDataParsers.get_parsers'), \
                mock.patch(CONFIG_NAMESPACE + '.ConfigMgr.is_ai_core_task_based', return_value=True):
            check = CalculatorFactory({}, CONFIG, '310')
            check.run()

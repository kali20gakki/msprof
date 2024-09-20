#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from constant.constant import CONFIG
from mscalculate.cann.cann_calculator import CANNCalculator

NAMESPACE = 'mscalculate.cann.cann_calculator'


class TestClusterLinkCalculate(unittest.TestCase):

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.CANNCalculator.calculate'),\
                mock.patch(NAMESPACE + '.CANNCalculator.save'):
            CANNCalculator({}, CONFIG).ms_run()
            with mock.patch('os.path.isfile', return_value=True), \
                mock.patch('os.path.islink', return_value=False), \
                mock.patch('os.access', return_value=True), \
                mock.patch('common_func.file_manager.is_other_writable', return_value=False), \
                mock.patch('common_func.file_manager.check_file_owner', return_value=True), \
                mock.patch('importlib.import_module'):
                CANNCalculator({}, CONFIG).ms_run()

    def test_calculate(self):
        InfoConfReader()._info_json = {'pid': '0'}
        with mock.patch('mscalculate.cann.cann_event_generator.CANNEventGenerator.run'), \
                mock.patch(NAMESPACE + '.CANNCalculator.save'):
            check = CANNCalculator({}, CONFIG)
            check.thread_set = {1, }
            check.calculate()

    def test_save(self):
        check = CANNCalculator({}, CONFIG)
        check.save()


if __name__ == '__main__':
    unittest.main()

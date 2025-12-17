#!/usr/bin/env python
# coding=utf-8
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
        with mock.patch(NAMESPACE + '.CANNCalculator.calculate'), \
                mock.patch(NAMESPACE + '.CANNCalculator.save'):
            CANNCalculator({}, CONFIG).ms_run()
            with mock.patch('os.path.isfile', return_value=True), \
                    mock.patch('os.path.islink', return_value=False), \
                    mock.patch('os.access', return_value=True), \
                    mock.patch('common_func.file_manager.is_other_writable', return_value=False), \
                    mock.patch('common_func.file_manager.check_file_owner', return_value=True), \
                    mock.patch('msinterface.msprof_c_interface.run_in_subprocess'), \
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

#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import struct

import unittest
from unittest import mock

from constant.constant import CONFIG
from common_func.empty_class import EmptyClass
from msparser.ge.ge_task_parser import GeTaskParser
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.ge.ge_task_parser'


class TestGeTaskParser(unittest.TestCase):
    def test_ms_run(self):
        check = GeTaskParser({}, CONFIG)
        check.ms_run()

        with mock.patch(NAMESPACE + '.GeTaskParser.parse'):
            check = GeTaskParser({DataTag.GE_STEP: ['Framework.task_desc_info.0.slice_0']}, CONFIG)
            check.ms_run()

    def test_parse(self):
        data = b'ZZ\x19\x00\x01\x00\x00\x00\x05\x00\x00\x00\x01\x00\x00\x00\\\xf1n\xf5\xa2Z\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\xe9\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
        with mock.patch('builtins.open', mock.mock_open(read_data=data)), \
             mock.patch('msparser.interface.data_parser.DataParser.get_file_path_and_check'), \
             mock.patch('os.path.getsize', return_value=256), \
             mock.patch('framework.offset_calculator.OffsetCalculator.pre_process', return_value=data):
            check = GeTaskParser({DataTag.GE_STEP: ['Framework.task_desc_info.0.slice_0']}, CONFIG)
            check.ms_run()

    def test_get_task_data(self):
        check = GeTaskParser({DataTag.GE_STEP: ['Framework.task_desc_info.0.slice_0']}, CONFIG)
        res = check._get_task_data(EmptyClass())
        self.assertEqual(res, [])

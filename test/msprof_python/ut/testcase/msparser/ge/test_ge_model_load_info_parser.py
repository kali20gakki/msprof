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
from msparser.ge.ge_model_load_info_parser import GeModelLoadParser
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.ge.ge_model_load_info_parser'


class TestGeModelLoadParser(unittest.TestCase):
    def test_ms_run(self):
        check = GeModelLoadParser({}, CONFIG)
        check.ms_run()

        with mock.patch(NAMESPACE + '.GeModelLoadParser.parse'):
            check = GeModelLoadParser({DataTag.GE_MODEL_LOAD: ['Framework.model_load_info_1.0.slice_0']}, CONFIG)
            check.ms_run()

    def test_parse(self):
        data = b'ZZ\x14\x00\x02\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00ge_default_20211216143802_31\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00r\x1bU\x88\xa3Z\x00\x00\xcc>\x82\x88\xa3Z\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
        with mock.patch('builtins.open', mock.mock_open(read_data=data)), \
                mock.patch('msparser.interface.data_parser.DataParser.get_file_path_and_check'), \
                mock.patch('os.path.getsize', return_value=256), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch('framework.offset_calculator.OffsetCalculator.pre_process', return_value=data):
            check = GeModelLoadParser({DataTag.GE_MODEL_LOAD: ['Framework.model_load_info_1.0.slice_0']}, CONFIG)
            check.parse()

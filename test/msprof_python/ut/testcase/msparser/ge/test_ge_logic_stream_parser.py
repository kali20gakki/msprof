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
from msparser.ge.ge_logic_stream_parser import GeLogicStreamParser
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.ge.ge_logic_stream_parser'


class TestGeLogicStreamInfoParser(unittest.TestCase):
    def test_ms_run(self):
        check = GeLogicStreamParser({}, CONFIG)
        check.ms_run()

        with mock.patch(NAMESPACE + '.GeLogicStreamParser.parse'):
            check = GeLogicStreamParser({DataTag.GE_LOGIC_STREAM_INFO: ['ge_logic_stream_info.1.slice_1']}, CONFIG)
            check.ms_run()

    def test_parse(self):
        ctx_data = (23130, 0, 0, 0, 0, 0, 1, 2, *(120,) * 48)
        struct_data = struct.pack("HHIIIQII48H", *ctx_data)
        with mock.patch('builtins.open', mock.mock_open(read_data=struct_data)), \
                mock.patch('msparser.interface.data_parser.DataParser.get_file_path_and_check'), \
                mock.patch('os.path.getsize', return_value=256), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch('framework.offset_calculator.OffsetCalculator.pre_process', return_value=struct_data):
            check = GeLogicStreamParser({DataTag.GE_FUSION_OP_INFO: ['ge_logic_stream_info.1.slice_1']}, CONFIG)
            check.parse()

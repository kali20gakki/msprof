#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import sqlite3
import struct
import unittest
from unittest import mock

from constant.constant import CONFIG
from msparser.ge.ge_host_parser import GeHostParser
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.ge.ge_host_parser'


class TestGeHostParser(unittest.TestCase):
    def test_ms_run(self):
        check = GeHostParser({}, CONFIG)
        check.ms_run()

        with mock.patch(NAMESPACE + '.GeHostParser.parse'), \
             mock.patch(NAMESPACE + '.GeHostParser.save',
                        side_effect=sqlite3.Error), \
             mock.patch(NAMESPACE + '.logging.error'):
            check = GeHostParser({DataTag.GE_HOST: ['Framework.dynamic_op_execute.0.slice_0']}, CONFIG)
            check.ms_run()

    def test_parse(self):
        data = struct.pack("=HHL4Q24B",
                           23130, 27, 1, 0, 123456789, 0, 10,
                           0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, 0)
        with mock.patch('builtins.open', mock.mock_open(read_data=data)), \
             mock.patch('os.path.getsize', return_value=200), \
             mock.patch(NAMESPACE + '.OffsetCalculator.pre_process'):
            check = GeHostParser({DataTag.GE_HOST: ['Framework.dynamic_op_execute.0.slice_0']}, CONFIG)
            check.parse()

    def test_save(self):
        data = struct.pack("=HHL4Q24B",
                           23130, 27, 1, 0, 123456789, 0, 10,
                           0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, 0)
        with mock.patch(NAMESPACE + '.GeHostParserModel.init'), \
             mock.patch(NAMESPACE + '.GeHostParserModel.flush'), \
             mock.patch(NAMESPACE + '.GeHostParserModel.finalize'):
            check = GeHostParser({DataTag.GE_HOST: ['Framework.dynamic_op_execute.0.slice_0']}, CONFIG)
            check._read_binary_data(data)
            check.save()

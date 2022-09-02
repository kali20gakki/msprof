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
from msparser.ge.ge_model_time_parser import GeModelTimeParser
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.ge.ge_model_time_parser'


class TestGeModelTimeParser(unittest.TestCase):
    def test_ms_run(self):
        check = GeModelTimeParser({}, CONFIG)
        check.ms_run()

        with mock.patch(NAMESPACE + '.GeModelTimeParser.parse'), \
                mock.patch(NAMESPACE + '.GeModelTimeParser.save', side_effect=sqlite3.Error), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = GeModelTimeParser({DataTag.GE_MODEL_TIME: ['Framework.model_time_info_1_0.0.slice_0']}, CONFIG)
            check.ms_run()

    def test_parse(self):
        data = b'ZZ\x16\x00\x01\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00ge_default_20211216143654_1\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xe9\x01\x00\x00\x86yl\xf5\xa2Z\x00\x00\xda\x8dm\xf5\xa2Z\x00\x00\x92\xc1m\xf5\xa2Z\x00\x00\xf4\xb9\x80\x05\xa3Z\x00\x00j\xee\x80\x05\xa3Z\x00\x00b+\x83\x05\xa3Z\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
        with mock.patch('builtins.open', mock.mock_open(read_data=data)), \
                mock.patch('msparser.interface.data_parser.DataParser.get_file_path_and_check'), \
                mock.patch('os.path.getsize', return_value=256), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch('framework.offset_calculator.OffsetCalculator.pre_process', return_value=data):
            check = GeModelTimeParser({DataTag.GE_MODEL_TIME: ['Framework.model_time_info_1_0.0.slice_0']}, CONFIG)
            check.parse()

    def test_save(self):
        with mock.patch(NAMESPACE + '.GeModelTimeModel.init'), \
                mock.patch(NAMESPACE + '.GeModelTimeModel.flush'), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch(NAMESPACE + '.GeModelTimeModel.finalize'):
            check = GeModelTimeParser({DataTag.GE_FUSION_OP_INFO: ['Framework.Framework.hash_dic.0.slice_0']}, CONFIG)
            check.save()

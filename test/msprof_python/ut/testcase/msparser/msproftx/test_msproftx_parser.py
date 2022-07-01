#!/usr/bin/env python
# coding=utf-8
"""
function: test msproftx_parser
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""

import struct
import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from constant.constant import CONFIG
from msparser.msproftx.msproftx_parser import MsprofTxParser
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.msproftx.msproftx_parser'


class TestMsprofTxParser(unittest.TestCase):
    file_list = {DataTag.MSPROFTX: ['msproftx.data.0.slice_0']}

    def test_read_binary_data(self):
        data = struct.pack('=HHLLLLLQQQLL128c9Q', 23130, 11, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's',
                           b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's',
                           b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's',
                           b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's',
                           b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's',
                           b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's',
                           b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's',
                           b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's',
                           b's', b's', b's', b's', b's', b's', b's', b's', 0, 0, 0, 0, 0, 0, 0, 0, 0)
        with mock.patch('os.path.join', return_value='test\\test'), \
                mock.patch('builtins.open', side_effect=OSError), \
                mock.patch(NAMESPACE + '.check_file_readable', return_value=True), \
                mock.patch('os.path.getsize', return_value=256), \
                mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = MsprofTxParser(self.file_list, CONFIG)
            result = check.read_binary_data('msproftx.data.5.slice_0')
        self.assertEqual(result, 1)
        with mock.patch('os.path.join', return_value='test\\test'), \
                mock.patch('builtins.open', mock.mock_open(read_data=data)), \
                mock.patch(NAMESPACE + '.check_file_readable', return_value=True), \
                mock.patch('os.path.getsize', return_value=256):
            InfoConfReader()._info_json = {'devices': '0'}
            check = MsprofTxParser(self.file_list, CONFIG)
            result = check.read_binary_data('msprotx.data.5.slice_0')
        self.assertEqual(result, 0)

    def test_original_data_handler(self):
        with mock.patch(NAMESPACE + '.logging.info'), \
                mock.patch(NAMESPACE + '.MsprofTxParser.read_binary_data', return_value=1), \
                mock.patch(NAMESPACE + '.FileManager.add_complete_file'), \
                mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = MsprofTxParser(self.file_list, CONFIG)
            check._original_data_handler('')

    def test_start_parsing_data_file(self):
        with mock.patch(NAMESPACE + '.is_valid_original_data', side_effect=OSError), \
                mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = MsprofTxParser(self.file_list, CONFIG)
            check.parse()
        with mock.patch(NAMESPACE + '.is_valid_original_data', return_value=True), \
                mock.patch(NAMESPACE + '.MsprofTxParser._original_data_handler',
                           return_value=0):
            InfoConfReader()._info_json = {'devices': '0'}
            check = MsprofTxParser(self.file_list, CONFIG)
            check.parse()

    def test_save(self):
        with mock.patch('msmodel.msproftx.msproftx_model.MsprofTxModel.init'), \
                mock.patch('msmodel.msproftx.msproftx_model.MsprofTxModel.create_table'), \
                mock.patch('msmodel.msproftx.msproftx_model.MsprofTxModel.flush'), \
                mock.patch('msmodel.msproftx.msproftx_model.MsprofTxModel.finalize'):
            InfoConfReader()._info_json = {"devices": '0'}
            check = MsprofTxParser(self.file_list, CONFIG)
            check.msproftx_data = [123]
            check.save()

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.MsprofTxParser.parse'), \
                mock.patch(NAMESPACE + '.MsprofTxParser.save', side_effect=OSError), \
                mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {'devices': '0'}
            MsprofTxParser(self.file_list, CONFIG).ms_run()

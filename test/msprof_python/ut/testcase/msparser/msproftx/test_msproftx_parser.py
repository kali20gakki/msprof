#!/usr/bin/env python
# coding=utf-8
"""
function: test msproftx_parser
Copyright Huawei Technologies Co., Ltd. 2020-2024. All rights reserved.
"""

import struct
import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from constant.constant import CONFIG
from msparser.data_struct_size_constant import StructFmt
from msparser.msproftx.msproftx_parser import MsprofTxParser
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.msproftx.msproftx_parser'


class TestMsprofTxParser(unittest.TestCase):
    file_list = {
        DataTag.MSPROFTX: ['msproftx.data.0.slice_0'],
        DataTag.MSPROFTX_EX: ['msproftx_ex.data.0.slice_0']
    }

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
                mock.patch('os.path.getsize', return_value=256), \
                mock.patch("common_func.file_manager.check_path_valid"), \
                mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = MsprofTxParser(self.file_list, CONFIG)
            result = check._read_tx_binary_data('msproftx.data.5.slice_0')
        self.assertEqual(result, 1)
        with mock.patch('os.path.join', return_value='test\\test'), \
                mock.patch('builtins.open', mock.mock_open(read_data=data)), \
                mock.patch("common_func.file_manager.check_path_valid"), \
                mock.patch('os.path.getsize', return_value=256):
            InfoConfReader()._info_json = {'devices': '0'}
            check = MsprofTxParser(self.file_list, CONFIG)
            result = check._read_tx_binary_data('msprotx.data.5.slice_0')
        self.assertEqual(result, 0)

    def test_read_tx_ex_bianary_data_return_error_with_fileopen_err(self):
        data = struct.pack('=HHLLLQQQQ128c', 23130, 3, 0, 0, 0, 0, 1, 10, 0,
                           b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's',
                           b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's',
                           b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's',
                           b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's',
                           b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's',
                           b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's',
                           b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's',
                           b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's',
                           b's', b's', b's', b's', b's', b's', b's', b'\0')
        with mock.patch('os.path.join', return_value='test\\test'), \
                mock.patch('builtins.open', side_effect=OSError), \
                mock.patch('os.path.getsize', return_value=256), \
                mock.patch("common_func.file_manager.check_path_valid"), \
                mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = MsprofTxParser(self.file_list, CONFIG)
            result = check._read_tx_ex_binary_data('msproftx_ex.slice_0')
        self.assertEqual(result, 1)

    def test_read_tx_ex__bianary_data_return_success_with_fileopen_succ(self):
        data = struct.pack('=HHLLLQQQQ128c', 23130, 3, 0, 0, 0, 0, 1, 10, 0,
                           b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's',
                           b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's',
                           b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's',
                           b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's',
                           b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's',
                           b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's',
                           b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's',
                           b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's', b's',
                           b's', b's', b's', b's', b's', b's', b's', b'\0')
        with mock.patch('os.path.join', return_value='test\\test'), \
                mock.patch('builtins.open', mock.mock_open(read_data=data)), \
                mock.patch("common_func.file_manager.check_path_valid"), \
                mock.patch('os.path.getsize', return_value=176):
            InfoConfReader()._info_json = {'devices': '0'}
            check = MsprofTxParser(self.file_list, CONFIG)
            result = check._read_tx_ex_binary_data('msproftx_ex.slice_0')
        self.assertEqual(result, 0)

    def test_original_tx_data_handler_with_read_err(self):
        with mock.patch(NAMESPACE + '.logging.info'), \
                mock.patch(NAMESPACE + '.MsprofTxParser._read_tx_binary_data', return_value=1), \
                mock.patch(NAMESPACE + '.FileManager.add_complete_file'), \
                mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = MsprofTxParser(self.file_list, CONFIG)
            check._file_tag = DataTag.MSPROFTX
            check._original_data_handler('')

    def test_original_tx_data_handler_with_read_succ(self):
        with mock.patch(NAMESPACE + '.logging.info'), \
                mock.patch(NAMESPACE + '.MsprofTxParser._read_tx_binary_data', return_value=0), \
                mock.patch(NAMESPACE + '.FileManager.add_complete_file'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = MsprofTxParser(self.file_list, CONFIG)
            check._file_tag = DataTag.MSPROFTX
            check._original_data_handler('')

    def test_original_tx_ex_data_handler_with_read_err(self):
        with mock.patch(NAMESPACE + '.logging.info'), \
                mock.patch(NAMESPACE + '.MsprofTxParser._read_tx_ex_binary_data', return_value=1), \
                mock.patch(NAMESPACE + '.FileManager.add_complete_file'), \
                mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = MsprofTxParser(self.file_list, CONFIG)
            check._file_tag = DataTag.MSPROFTX_EX
            check._original_data_handler('')

    def test_original_tx_ex_data_handler_with_read_succ(self):
        with mock.patch(NAMESPACE + '.logging.info'), \
                mock.patch(NAMESPACE + '.MsprofTxParser._read_tx_ex_binary_data', return_value=1), \
                mock.patch(NAMESPACE + '.FileManager.add_complete_file'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = MsprofTxParser(self.file_list, CONFIG)
            check._file_tag = DataTag.MSPROFTX_EX
            check._original_data_handler('')

    def test_parse_with_original_handle_succ(self):
        with mock.patch(NAMESPACE + '.is_valid_original_data', return_value=True), \
                mock.patch(NAMESPACE + '.MsprofTxParser._original_data_handler',
                           return_value=0):
            InfoConfReader()._info_json = {'devices': '0'}
            check = MsprofTxParser(self.file_list, CONFIG)
            check.parse()

    def test_parse_with_original_handle_err_by_invalid_file_tag(self):
        with mock.patch(NAMESPACE + '.is_valid_original_data', return_value=True):
            InfoConfReader()._info_json = {'devices': '0'}
            check = MsprofTxParser(self.file_list, CONFIG)
            check._file_tag = DataTag.ACL
            check.parse()

    def test_save_with_msproftx_data(self):
        with mock.patch('msmodel.msproftx.msproftx_model.MsprofTxModel.flush'):
            InfoConfReader()._info_json = {"devices": '0'}
            check = MsprofTxParser(self.file_list, CONFIG)
            check._msproftx_data = [123]
            check.save()

    def test_save_with_msproftx_data(self):
        with mock.patch('msmodel.msproftx.msproftx_model.MsprofTxExModel.flush'):
            InfoConfReader()._info_json = {"devices": '0'}
            check = MsprofTxParser(self.file_list, CONFIG)
            check._msproftx_ex_data = [123]
            check.save()

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.MsprofTxParser.parse'), \
                mock.patch(NAMESPACE + '.MsprofTxParser.save', side_effect=OSError), \
                mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {'devices': '0'}
            MsprofTxParser(self.file_list, CONFIG).ms_run()


#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
"""
import struct
import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from constant.constant import CONFIG
from common_func.ms_constant.number_constant import NumberConstant
from msparser.data_struct_size_constant import StructFmt
from msparser.msproftx.msproftx_ex_parser import MsprofTxExParser
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.struct_info.msproftx_decoder import MsprofTxExDecoder

NAMESPACE: str = 'msparser.msproftx.msproftx_ex_parser'


class TestMsprofTxExParser(unittest.TestCase):
    file_list = {DataTag.MSPROFTX_EX: ['Msprof.msproftx_ex.slice_0']}

    def test_read_bianary_data_return_error_with_fileopen_err(self):
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
            check = MsprofTxExParser(self.file_list, CONFIG)
            result = check._read_binary_data('msproftx_ex.slice_0')
        self.assertEqual(result, NumberConstant.ERROR)

    def test_read_bianary_data_return_success_with_fileopen_succ(self):
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
            check = MsprofTxExParser(self.file_list, CONFIG)
            result = check._read_binary_data('msproftx_ex.slice_0')
        self.assertEqual(result, NumberConstant.SUCCESS)

    def test_original_data_handler_will_log_err_with_read_binary_data_err(self):
        with mock.patch(NAMESPACE + '.logging.info'), \
                mock.patch(NAMESPACE + '.MsprofTxExParser._read_binary_data', return_value=NumberConstant.ERROR), \
                mock.patch(NAMESPACE + '.FileManager.add_complete_file'), \
                mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = MsprofTxExParser(self.file_list, CONFIG)
            check._original_data_handler('')

    def test_original_data_handler_will_log_info_with_read_binary_data_succ(self):
        with mock.patch(NAMESPACE + '.logging.info'), \
                mock.patch(NAMESPACE + '.MsprofTxExParser._read_binary_data', return_value=0), \
                mock.patch(NAMESPACE + '.FileManager.add_complete_file'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = MsprofTxExParser(self.file_list, CONFIG)
            check._original_data_handler('')

    def test_ms_run_will_return_with_no_file_list(self):
        file_dict_with_no_ex_file_list = {}
        MsprofTxExParser(file_dict_with_no_ex_file_list, CONFIG).ms_run()

    def test_ms_run_with_file_list(self):
        with mock.patch(NAMESPACE + '.MsprofTxExParser.parse'), \
                mock.patch(NAMESPACE + '.MsprofTxExParser.save'):
            MsprofTxExParser(self.file_list, CONFIG).ms_run()

    def test_parse_with_invalid_file_list(self):
        with mock.patch(NAMESPACE + '.is_valid_original_data', return_value=False):
            MsprofTxExParser(self.file_list, CONFIG).parse()

    def test_parse_with_valid_file_list(self):
        with mock.patch(NAMESPACE + '.is_valid_original_data', return_value=True), \
                mock.patch(NAMESPACE + '.MsprofTxExParser._original_data_handler'):
            MsprofTxExParser(self.file_list, CONFIG).parse()

    def test_save_with_ex_data(self):
        with mock.patch('msmodel.msproftx.msproftx_model.MsprofTxExModel.flush'):
            check = MsprofTxExParser(self.file_list, CONFIG)
            check._msproftx_ex_data = [(0, 0, 'marker_ex', 10, 10, 0, 'test')]
            check.save()
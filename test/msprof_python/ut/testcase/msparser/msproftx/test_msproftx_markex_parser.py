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
from msparser.msproftx.msproftx_markex_parser import MsprofTxMarkExParser
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.struct_info.msproftx_decoder import MsprofTxMarkExDecoder

NAMESPACE: str = 'msparser.msproftx.msproftx_markex_parser'


class TestMsprofTxMarkExParser(unittest.TestCase):
    file_list = {DataTag.MSPROFTX_MARKEX: ['Msprof.mark_ex.slice_0']}

    def test_read_bianary_data_return_error_with_fileopen_err(self):
        data = struct.pack('=HHLLLQQ128c', 23130, 0, 0, 0, 0, 0, 1,
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
            check = MsprofTxMarkExParser(self.file_list, CONFIG)
            result = check._read_binary_data('mark_ex.slice_0')
        self.assertEqual(result, NumberConstant.ERROR)

    def test_read_bianary_data_return_success_with_fileopen_succ(self):
        data = struct.pack('=HHLLLQQ128c', 23130, 0, 0, 0, 0, 0, 1,
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
                mock.patch('os.path.getsize', return_value=256):
            InfoConfReader()._info_json = {'devices': '0'}
            check = MsprofTxMarkExParser(self.file_list, CONFIG)
            result = check._read_binary_data('mark_ex.slice_0')
        self.assertEqual(result, NumberConstant.SUCCESS)

    def test_original_data_handler_will_log_err_with_read_binary_data_err(self):
        with mock.patch(NAMESPACE + '.logging.info'), \
                mock.patch(NAMESPACE + '.MsprofTxMarkExParser._read_binary_data', return_value=NumberConstant.ERROR), \
                mock.patch(NAMESPACE + '.FileManager.add_complete_file'), \
                mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = MsprofTxMarkExParser(self.file_list, CONFIG)
            check._original_data_handler('')

    def test_original_data_handler_will_log_info_with_read_binary_data_succ(self):
        with mock.patch(NAMESPACE + '.logging.info'), \
                mock.patch(NAMESPACE + '.MsprofTxMarkExParser._read_binary_data', return_value=0), \
                mock.patch(NAMESPACE + '.FileManager.add_complete_file'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = MsprofTxMarkExParser(self.file_list, CONFIG)
            check._original_data_handler('')

    def test_ms_run_will_return_with_no_file_list(self):
        file_dict_with_no_markex_file_list = {}
        MsprofTxMarkExParser(file_dict_with_no_markex_file_list, CONFIG).ms_run()

    def test_ms_run_with_file_list(self):
        with mock.patch(NAMESPACE + '.MsprofTxMarkExParser.parse'), \
                mock.patch(NAMESPACE + '.MsprofTxMarkExParser.save'):
            MsprofTxMarkExParser(self.file_list, CONFIG).ms_run()

    def test_parse_with_invalid_file_list(self):
        with mock.patch(NAMESPACE + '.is_valid_original_data', return_value=False):
            MsprofTxMarkExParser(self.file_list, CONFIG).parse()

    def test_parse_with_valid_file_list(self):
        with mock.patch(NAMESPACE + '.is_valid_original_data', return_value=True), \
                mock.patch(NAMESPACE + '.MsprofTxMarkExParser._original_data_handler'):
            MsprofTxMarkExParser(self.file_list, CONFIG).parse()

    def test_save_with_markex_data(self):
        with mock.patch('msmodel.msproftx.msproftx_model.MsprofTxMarkExModel.init'), \
                mock.patch('msmodel.msproftx.msproftx_model.MsprofTxMarkExModel.create_table'), \
                mock.patch('msmodel.msproftx.msproftx_model.MsprofTxMarkExModel.flush'), \
                mock.patch('msmodel.msproftx.msproftx_model.MsprofTxMarkExModel.finalize'):
            check = MsprofTxMarkExParser(self.file_list, CONFIG)
            check._msproftx_markex_data = [(0, 0, 0, 1, 'test')]
            check.save()
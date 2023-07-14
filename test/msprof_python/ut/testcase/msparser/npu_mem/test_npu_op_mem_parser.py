#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

import struct
import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from constant.constant import CONFIG
from msparser.npu_mem.npu_op_mem_parser import NpuOpMemParser
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.npu_mem.npu_op_mem_parser'


class TestNpuOpMemParser(unittest.TestCase):
    file_list = {DataTag.MEMORY_OP: ['aging.additional.task_memory_info.slice_0']}

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.NpuOpMemParser.parse'), \
                mock.patch(NAMESPACE + '.NpuOpMemParser.save'):
            check = NpuOpMemParser(self.file_list, CONFIG)
            check.ms_run()
        with mock.patch(NAMESPACE + '.NpuOpMemParser.parse', side_effect=SystemError), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = NpuOpMemParser(self.file_list, CONFIG)
            check.ms_run()

    def test_save(self):
        with mock.patch(NAMESPACE + '.NpuOpMemModel.clear'), \
                mock.patch(NAMESPACE + '.NpuOpMemModel.init'), \
                mock.patch(NAMESPACE + '.NpuOpMemModel.flush'), \
                mock.patch(NAMESPACE + '.NpuOpMemModel.finalize'):
            check = NpuOpMemParser(self.file_list, CONFIG)
            setattr(check, "_npu_op_mem_data", ['123', '1', 1, '123', 1, 1000, 1000, 1, 0, 'NPU:0'])
            result = check.save()
        with mock.patch(NAMESPACE + '.NpuOpMemModel.init'), \
                mock.patch(NAMESPACE + '.NpuOpMemModel.flush'), \
                mock.patch(NAMESPACE + '.NpuOpMemModel.finalize'):
            check = NpuOpMemParser(self.file_list, CONFIG)
            setattr(check, "_npu_op_mem_data", [])
            result = check.save()

    def test_parse(self):
        with mock.patch(NAMESPACE + '.PathManager.get_data_file_path', return_value='test'), \
                mock.patch('os.path.getsize', return_value=0):
            check = NpuOpMemParser(self.file_list, CONFIG)
            check.parse()
        with mock.patch('os.path.getsize', return_value=128), \
                mock.patch(NAMESPACE + '.PathManager.get_data_file_path', return_value='test'), \
                mock.patch(NAMESPACE + '.logging.info'), \
                mock.patch(NAMESPACE + '.FileManager.add_complete_file'), \
                mock.patch(NAMESPACE + '.NpuOpMemParser._process_npu_op_mem_data'):
            check = NpuOpMemParser(self.file_list, CONFIG)
            check.parse()

    def test_process_npu_op_mem_data(self):
        npu_op_mem_data_new = struct.pack(
            '=HHIIIQQqQQQII184B', 1, 1, 0, 1, 15, 123,
            0x00000001, 1, 123, 1000, 1000, 0, 123,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0
        )
        InfoConfReader()._info_json = {'CPU': [{'Frequency': "1000"}]}
        with mock.patch('builtins.open', mock.mock_open(read_data=npu_op_mem_data_new)), \
                mock.patch(NAMESPACE + '.FileOpen'), \
                mock.patch(NAMESPACE + '.OffsetCalculator.pre_process', return_value=npu_op_mem_data_new):
            check = NpuOpMemParser(self.file_list, CONFIG)
            check._process_npu_op_mem_data('test', 256, [])
        self.assertEqual(check._npu_op_mem_data, [['123', '1', 1, '123', 1, 1000, 1000, 1, 0, 'NPU:0']])

#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

import struct
import unittest
from unittest import mock

from constant.constant import CONFIG
from msparser.npu_mem.npu_mem_parser import NpuMemParser
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.npu_mem.npu_mem_parser'


class TestNpuMemParser(unittest.TestCase):
    file_list = {DataTag.NPU_MEM: ['npu.app.mem.data.0.slice_0', 'npu.mem.data.0.slice_0']}

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.NpuMemParser.parse'), \
                mock.patch(NAMESPACE + '.NpuMemParser.save'):
            check = NpuMemParser(self.file_list, CONFIG)
            check.ms_run()
        with mock.patch(NAMESPACE + '.NpuMemParser.parse', side_effect=SystemError), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = NpuMemParser(self.file_list, CONFIG)
            check.ms_run()

    def test_save(self):
        with mock.patch(NAMESPACE + '.NpuMemModel.init'), \
                mock.patch(NAMESPACE + '.NpuMemModel.flush'), \
                mock.patch(NAMESPACE + '.NpuMemModel.finalize'):
            check = NpuMemParser(self.file_list, CONFIG)
            setattr(check, "_npu_mem_data", [1, 0, 128, 6, 128, 0])
            result = check.save()
        with mock.patch(NAMESPACE + '.NpuMemModel.init'), \
                mock.patch(NAMESPACE + '.NpuMemModel.flush'), \
                mock.patch(NAMESPACE + '.NpuMemModel.finalize'):
            check = NpuMemParser(self.file_list, CONFIG)
            setattr(check, "_npu_mem_data", [])
            result = check.save()

    def test_parse(self):
        with mock.patch(NAMESPACE + '.PathManager.get_data_file_path', return_value='test'), \
                mock.patch('os.path.getsize', return_value=0):
            check = NpuMemParser(self.file_list, CONFIG)
            check.parse()
        with mock.patch('os.path.getsize', return_value=128), \
                mock.patch(NAMESPACE + '.PathManager.get_data_file_path', return_value='test'), \
                mock.patch(NAMESPACE + '.logging.info'), \
                mock.patch(NAMESPACE + '.FileManager.add_complete_file'), \
                mock.patch(NAMESPACE + '.NpuMemParser._process_npu_mem_data'):
            check = NpuMemParser(self.file_list, CONFIG)
            check.parse()

    def test_process_npu_mem_data(self):
        npu_mem_data_new = struct.pack('=IIQQQ', 6, 0, 0, 16, 0)
        with mock.patch('builtins.open', mock.mock_open(read_data=npu_mem_data_new)), \
                mock.patch(NAMESPACE + '.FileOpen'), \
                mock.patch(NAMESPACE + '.OffsetCalculator.pre_process', return_value=npu_mem_data_new):
            check = NpuMemParser(self.file_list, CONFIG)
            check._process_npu_mem_data('test', 32, [])
        self.assertEqual(check._npu_mem_data, [[0, 16, 0, 6, 16]])

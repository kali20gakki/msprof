#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

import struct
import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from msparser.freq.freq_parser import FreqParser
from msparser.data_struct_size_constant import StructFmt
from profiling_bean.prof_enum.data_tag import DataTag
from framework.offset_calculator import OffsetCalculator
from constant.constant import CONFIG

NAMESPACE = 'msparser.freq.freq_parser'


class TestFreqParser(unittest.TestCase):
    file_list = {DataTag.FREQ: ['lpmFreqConv.data.0.slice_0']}

    def test_parse(self):
        with mock.patch(NAMESPACE + '.PathManager.get_data_file_path', return_value='test'), \
                mock.patch(NAMESPACE + '.FreqParser._read_file'), \
                mock.patch('os.path.getsize', return_value=408), \
                mock.patch(NAMESPACE + '.FileManager.add_complete_file'):
            check = FreqParser(self.file_list, CONFIG)
            check.parse()
        with mock.patch(NAMESPACE + '.PathManager.get_data_file_path', return_value='test'), \
                mock.patch(NAMESPACE + '.FreqParser._read_file'), \
                mock.patch('os.path.getsize', return_value=0), \
                mock.patch(NAMESPACE + '.FileManager.add_complete_file'):
            check = FreqParser(self.file_list, CONFIG)
            check.parse()

    def test_read_file(self):
        data = struct.pack(StructFmt.FREQ_FMT, 2, 0, 100, 10, 0, 120, 30, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
        offset_calculator = OffsetCalculator(self.file_list.get(DataTag.FREQ), StructFmt.FREQ_DATA_SIZE, 'test')
        with mock.patch('builtins.open', mock.mock_open(read_data=data)),\
                mock.patch(NAMESPACE + '.OffsetCalculator.pre_process', return_value=data),\
                mock.patch(NAMESPACE + '.FileManager.add_complete_file'):
            check = FreqParser(self.file_list, CONFIG)
            check._read_file('test', 408, offset_calculator)

    def test_save(self):
        with mock.patch('msmodel.freq.freq_parser_model.FreqParserModel.init'), \
                mock.patch('msmodel.freq.freq_parser_model.FreqParserModel.check_db'), \
                mock.patch('msmodel.freq.freq_parser_model.FreqParserModel.create_table'), \
                mock.patch('msmodel.freq.freq_parser_model.FreqParserModel.insert_data_to_db'), \
                mock.patch('msmodel.freq.freq_parser_model.FreqParserModel.finalize'):
            InfoConfReader()._info_json = {"devices": '0'}
            check = FreqParser(self.file_list, CONFIG)
            check._freq_data = [[100, 10], [120, 30]]
            check.save()

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.FreqParser.parse'), \
                mock.patch(NAMESPACE + '.FreqParser.save'):
            FreqParser(self.file_list, CONFIG).ms_run()
        FreqParser({DataTag.FREQ: []}, CONFIG).ms_run()

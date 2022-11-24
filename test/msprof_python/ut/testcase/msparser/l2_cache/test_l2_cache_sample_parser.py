#!/usr/bin/env python
# coding=utf-8
"""
function: test l2_cache_parser
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import struct
import unittest
from unittest import mock

from msparser.l2_cache.l2_cache_sample_parser import L2CacheSampleParser
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.l2_cache.l2_cache_sample_parser'
MODEL_NAMESPACE = 'msmodel.l2_cache.l2_cache_parser_model'
BEAN_NAMESPACE = 'profiling_bean.struct_info.l2_cache'


class TestL2CacheSampleParser(unittest.TestCase):
    sample_config = {'l2CacheTaskProfiling': 'on',
                     'l2CacheTaskProfilingEvents': '0x78,0x79,0x77,0x71,0x6A,0x6C,0x74,0x62',
                     'result_dir': '/tmp/result',
                     'tag_id': 'JOBEJGBAHABDEEIJEDFHHFAAAAAAAAAA',
                     'host_id': '127.0.0.1',
                     'ai_core_profiling_mode': 'sample-based'
                     }
    file_list = {DataTag.L2CACHE: ['l2_cache.data.0.slice_0']}

    def test_ms_run(self):
        data = struct.pack("=BBH4B8Q", 0, 3, 26, 0, 2, 3, 91, 98, 0, 0, 505, 1, 1089, 0, 106)
        with mock.patch(NAMESPACE + '.PathManager.get_data_file_path', return_value=144), \
                mock.patch(MODEL_NAMESPACE + '.L2CacheParserModel.init'), \
                mock.patch(MODEL_NAMESPACE + '.L2CacheParserModel.flush'), \
                mock.patch(MODEL_NAMESPACE + '.L2CacheParserModel.finalize'),\
                mock.patch('builtins.open', mock.mock_open(read_data=data)), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch('os.path.getsize', return_value=72):
            check = L2CacheSampleParser(self.file_list, self.sample_config)
            check.parse()
            check = L2CacheSampleParser(self.file_list, self.sample_config)
            check.ms_run()

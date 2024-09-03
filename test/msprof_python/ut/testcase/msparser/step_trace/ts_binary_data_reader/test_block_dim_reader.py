#!/usr/bin/python3
# -*- coding: utf-8 -*-
#  Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
import struct
import unittest

from msparser.step_trace.ts_binary_data_reader.block_dim_reader import BlockDimReader


class TestBlockDimReader(unittest.TestCase):
    def test_read_binary_data_should_get_block_dim_when_binary_data_parsed(self):
        # stream_id, task_id, timestamp, block_dim
        expect_res = [(123456789, 1, 1, 8)]

        file_data = struct.pack("=BBH4BQHHL2Q", 1, 15, 32, 0, 0, 0, 0, 123456789, 1, 1, 8, 0, 0)

        block_dim_reader = BlockDimReader()
        block_dim_reader.read_binary_data(file_data)
        self.assertEqual(expect_res, block_dim_reader.data)

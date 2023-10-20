#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
import struct
import unittest
from unittest import mock

from msparser.step_trace.ts_binary_data_reader.task_flip_reader import TaskFlipReader
from common_func.info_conf_reader import InfoConfReader


class TestTaskFlipReader(unittest.TestCase):

    def test_read_binary_data_should_return_tasks_len_4_when_decode(self) -> None:
        data = [1, 14, 1, 0, 111111111000, 2, 1, 0, 1, *(0,) * 16]
        struct_data = struct.pack("=BBHLQHHHH16B", *data)
        InfoConfReader()._info_json = {
            "DeviceInfo": [{"hwts_frequency": 100}]
        }

        task_flip_reader = TaskFlipReader()
        task_flip_reader.read_binary_data(struct_data)
        self.assertEqual(task_flip_reader.table_name, 'DeviceTaskFlip')
        self.assertEqual(len(task_flip_reader.data), 1)
        self.assertEqual(len(task_flip_reader.data[0]), 4)

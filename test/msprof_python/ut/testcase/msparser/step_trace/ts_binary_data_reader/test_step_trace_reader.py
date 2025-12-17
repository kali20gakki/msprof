# -------------------------------------------------------------------------
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
# This file is part of the MindStudio project.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#    http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------
import struct
import unittest

from msparser.step_trace.ts_binary_data_reader.step_trace_reader import StepTraceReader


class TestStepTraceReader(unittest.TestCase):
    def test_read_binary_data(self):
        # index id, model id, timestamp, stream_id, task_id, tag_id
        expect_res = [(1, 1, 101612167908, 3, 1, 0)]

        file_data = struct.pack("=BBH4BQQQHHHH", 1, 10, 32, 0, 0, 0, 0, 101612167908, 1, 1, 3, 1, 0, 0)

        step_trace_reader = StepTraceReader()
        step_trace_reader.read_binary_data(file_data)
        self.assertEqual(expect_res, step_trace_reader.data)

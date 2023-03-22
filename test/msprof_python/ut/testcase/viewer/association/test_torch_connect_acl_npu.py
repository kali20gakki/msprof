#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2023. All rights reserved.

import unittest
from unittest import mock

from profiling_bean.db_dto.torch_npu_dto import TorchNpuDto
from viewer.association.torch_connect_acl_npu import TorchToAclNpu

NAMESPACE = "viewer.association.torch_connect_acl_npu"


class TestTorchToAclNpu(unittest.TestCase):
    data1 = TorchNpuDto()
    data1.torch_op_start_time = 1000
    data1.torch_op_pid = 1
    data1.torch_op_tid = 1
    data1.acl_start_time = 1500
    data1.stream_id = 5
    data1.task_id = 3
    data1.batch_id = 0
    json_data = [
        {"name": "AscendCL@aclopCompileAndExecute", "pid": "2_1071532", "tid": 1071819, "ts": 1.5, "dur": 5, "ph": "X"},
        {"name": "Mul", "pid": "3_0", "tid": 5, "ts": 3, "dur": 3.4,
         "args": {"Task Type": "AI_CORE", "Stream Id": 5, "Task Id": 3, "Batch Id": 0, "Aicore Time(ms)": 2681},
         "ph": "X"}
    ]
    check_data = [
        {'name': 'AscendCL@aclopCompileAndExecute', 'pid': '2_1071532', 'tid': 1071819, 'ts': 1.5, 'dur': 5, 'ph': 'X'},
        {'name': 'Mul', 'pid': '3_0', 'tid': 5, 'ts': 3, 'dur': 3.4,
         'args': {'Task Type': 'AI_CORE', 'Stream Id': 5, 'Task Id': 3, "Batch Id": 0, 'Aicore Time(ms)': 2681},
         'ph': 'X'},
        {'name': 'torch_to_acl', 'ph': 's', 'id': 1.5, 'pid': '0_1', 'tid': 1, 'ts': 1.0, 'cat': 'async_acl_npu'},
        {'name': 'torch_to_acl', 'ph': 'f', 'id': 1.5, 'pid': '2_1071532', 'tid': 1071819, 'ts': 1.5, 'bp': 'e',
         'cat': 'async_acl_npu'},
        {'name': 'torch_to_npu', 'ph': 's', 'id': '5_3_0', 'pid': '0_1', 'tid': 1, 'ts': 1.0, 'cat': 'async_npu'},
        {'name': 'torch_to_npu', 'ph': 'f', 'id': '5_3_0', 'pid': '3_0', 'tid': 5, 'ts': 3, 'bp': 'e',
         'cat': 'async_npu'}
    ]

    def test_add_connect_line_1(self):
        with mock.patch(NAMESPACE + '.SyncAclNpuViewModel.get_torch_acl_relation_data', return_value=[self.data1]), \
                mock.patch(NAMESPACE + '.PathManager.get_db_path'), \
                mock.patch(NAMESPACE + '.SyncAclNpuViewModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + '.SyncAclNpuViewModel.get_torch_npu_relation_data', return_value=[self.data1]):
            check = TorchToAclNpu("")
            check.add_connect_line(self.json_data)
            self.assertEqual(self.json_data, self.check_data)

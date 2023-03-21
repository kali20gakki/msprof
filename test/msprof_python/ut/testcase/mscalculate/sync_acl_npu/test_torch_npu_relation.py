#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2023. All rights reserved.

import unittest
from unittest import mock

from mscalculate.sync_acl_npu.torch_npu_relation import TorchNpuRelationCalculator
from profiling_bean.db_dto.ge_task_dto import GeTaskDto
from profiling_bean.db_dto.torch_npu_dto import TorchNpuDto

NAMESPACE = "mscalculate.sync_acl_npu.torch_npu_relation"


class TestTorchNpuRelationCalculator(unittest.TestCase):
    torch_acl_data1 = TorchNpuDto()
    torch_acl_data1.acl_compile_time = 1
    torch_acl_data1.acl_start_time = 2
    torch_acl_data1.acl_end_time = 4
    torch_acl_data1.op_name = "add"
    torch_acl_data1.torch_op_start_time = 0

    ge_data = GeTaskDto()
    ge_data.stream_id = 1
    ge_data.task_id = 1
    ge_data.batch_id = 0
    ge_data.context_id = 5
    ge_data.timestamp = 3

    def test_ms_run(self):
        with mock.patch(NAMESPACE + ".SyncAclNpuViewModel.get_torch_acl_relation_data",
                        return_value=[self.torch_acl_data1]), \
                mock.patch(NAMESPACE + ".PathManager.get_db_path"), \
                mock.patch(NAMESPACE + ".SyncAclNpuViewModel.check_table", return_value=True), \
                mock.patch(NAMESPACE + ".ViewModel.check_table", return_value=True), \
                mock.patch(NAMESPACE + ".DBManager.fetch_all_data", return_value=[self.ge_data]), \
                mock.patch(NAMESPACE + ".SyncAclNpuModel.flush"):
            check = TorchNpuRelationCalculator({}, {"result_dir": "d:\\test\\device_0"})
            check.ms_run()
            self.assertEqual(check.torch_npu_relation_data,
                             [[0, None, None, 'add', 2, 4, None, 1, 1, 1, 0, 5, None, None, 1]])

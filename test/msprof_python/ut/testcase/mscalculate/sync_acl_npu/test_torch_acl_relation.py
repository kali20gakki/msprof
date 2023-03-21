#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2023. All rights reserved.

import unittest
from unittest import mock

from mscalculate.sync_acl_npu.torch_acl_relation import TorchAclRelationCalculator
from profiling_bean.db_dto.acl_dto import AclDto
from profiling_bean.db_dto.msproftx_dto import MsprofTxDto

NAMESPACE = "mscalculate.sync_acl_npu.torch_acl_relation"


class TestTorchAclRelationCalculator(unittest.TestCase):
    torch_op1 = MsprofTxDto()
    torch_op1.pid = 1
    torch_op1.tid = 1
    torch_op1.start_time = 0
    torch_op1.end_time = 1
    torch_op1.message = 'add'

    torch_op2 = MsprofTxDto()
    torch_op2.pid = 1
    torch_op2.tid = 1
    torch_op2.start_time = 3
    torch_op2.end_time = 4
    torch_op2.message = 'add'

    mark1 = MsprofTxDto()
    mark1.start_time = 1
    mark1.message = 'add'

    mark2 = MsprofTxDto()
    mark2.start_time = 5
    mark2.message = 'add'

    acl_op_exe1 = AclDto()
    acl_op_exe1.start_time = 2
    acl_op_exe1.end_time = 4
    acl_op_exe1.thread_id = 1

    acl_op_exe2 = AclDto()
    acl_op_exe2.start_time = 6
    acl_op_exe2.end_time = 9
    acl_op_exe2.thread_id = 1

    acl_op_compile1 = AclDto()
    acl_op_compile1.start_time = 2
    acl_op_compile1.end_time = 3

    def test_ms_run(self):
        with mock.patch(NAMESPACE + ".DataCheckManager.contain_info_json_data", return_value=True), \
                mock.patch(NAMESPACE + ".FileManager.is_analyzed_data"), \
                mock.patch(NAMESPACE + ".analyze_collect_data"), \
                mock.patch(NAMESPACE + ".ConfigMgr.read_sample_config"), \
                mock.patch(NAMESPACE + ".MsprofTxModel.get_torch_op_data",
                           return_value=[self.torch_op1, self.torch_op2]), \
                mock.patch(NAMESPACE + ".MsprofTxModel.get_mark_data",
                           return_value=[self.mark2, self.mark1]), \
                mock.patch(NAMESPACE + ".PathManager.get_db_path"), \
                mock.patch(NAMESPACE + ".MsprofTxModel.check_table", return_value=True), \
                mock.patch(NAMESPACE + ".AclModel.get_acl_op_execute_data",
                           return_value=[self.acl_op_exe1, self.acl_op_exe2]), \
                mock.patch(NAMESPACE + ".AclModel.check_db", return_value=True), \
                mock.patch(NAMESPACE + ".AclModel.check_table", return_value=True), \
                mock.patch(NAMESPACE + ".AclModel.get_acl_op_compile_data", return_value=[self.acl_op_compile1]):
            check = TorchAclRelationCalculator({}, {"result_dir": "d:\\test\\device_0"})
            check.ms_run()
            self.assertEqual(check._matched_torch_acl_data,
                             [['add', 1, 2, 4, 1, 1, 0, 1, 1], ['add', 5, 6, 9, 1, 0, 3, 1, 1]])

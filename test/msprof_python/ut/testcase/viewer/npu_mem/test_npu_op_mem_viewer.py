#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

import unittest
from unittest import mock

from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.msvp_constant import MsvpConstant
from profiling_bean.db_dto.op_mem_dto import OpMemDto
from profiling_bean.db_dto.npu_op_mem_dto import NpuOpMemDto
from profiling_bean.db_dto.npu_op_mem_rec_dto import NpuOpMemRecDto
from viewer.npu_mem.npu_op_mem_viewer import NpuOpMemViewer

NAMESPACE = 'viewer.npu_mem.npu_op_mem_viewer'


class TestNpuOpMemViewer(unittest.TestCase):

    def test_get_summary_data_should_return_empty_when_model_init_fail(self):
        config = {"headers": ["component", "timestamp", "total_reserve_memory", "total_allocate_memory", "device_type"]}
        params = {
            "project": "test_npu_mem_view",
            "model_id": 1,
            "iter_id": 1
        }
        check = NpuOpMemViewer(config, params, DBNameConstant.TABLE_NPU_OP_MEM_REC)
        ret = check.get_summary_data()
        self.assertEqual(MsvpConstant.MSVP_EMPTY_DATA, ret)

    def test_get_summary_data_should_return_success_when_get_rec_data(self):
        config = {"headers": ["component", "timestamp", "total_reserve_memory", "total_allocate_memory", "device_type"]}
        params = {
            "project": "test_npu_op_mem_view",
            "model_id": 1,
            "iter_id": 1
        }
        rec_dto = NpuOpMemRecDto()
        rec_dto.component = 'GE'
        rec_dto.timestamp = 0
        rec_dto.total_reserve_memory = 0
        rec_dto.total_allocate_memory = 0
        rec_dto.device_type = 'NPU:5'
        with mock.patch(NAMESPACE + '.NpuOpMemModel.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.NpuOpMemModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + '.NpuOpMemModel.get_summary_data', return_value=[rec_dto]):
            check = NpuOpMemViewer(config, params, '2')
            ret = check.get_summary_data()

    def test_get_summary_data_should_return_success_when_get_mem_data(self):
        config = {
            "headers": ["operator", "size", "allocation_time", "release_time", "duration",
                              "allocation_total_allocated", "allocation_total_reserved", "release_total_allocated",
                              "release_total_reserved", "device_type", "name"]
        }
        params = {
            "project": "test_npu_op_mem_view",
            "model_id": 1,
            "iter_id": 1
        }
        mem_dto = OpMemDto()
        mem_dto.operator = '123'
        mem_dto.size = 0
        mem_dto.allocation_time = 0
        mem_dto.release_time = 0
        mem_dto.duration = 1
        mem_dto.allocation_total_allocated = 0
        mem_dto.allocation_total_reserved = 0
        mem_dto.release_total_allocated = 0
        mem_dto.release_total_reserved = 0
        mem_dto.device_type = 'NPU:5'
        mem_dto.name = '123'

        with mock.patch(NAMESPACE + '.NpuOpMemModel.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.NpuOpMemModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + '.NpuOpMemModel.get_summary_data', return_value=[mem_dto]):
            check = NpuOpMemViewer(config, params, '2')
            ret = check.get_summary_data()
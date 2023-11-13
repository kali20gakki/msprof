#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

import unittest
from unittest import mock

from common_func.db_name_constant import DBNameConstant
from common_func.msvp_constant import MsvpConstant
from common_func.info_conf_reader import InfoConfReader
from profiling_bean.db_dto.npu_op_mem_rec_dto import NpuOpMemRecDto
from profiling_bean.db_dto.npu_module_mem_dto import NpuModuleMemDto
from viewer.npu_mem.npu_mem_rec_viewer import NpuMemRecViewer
from model.test_dir_cr_base_model import TestDirCRBaseModel

NAMESPACE = 'viewer.npu_mem.npu_mem_rec_viewer'


class TestNpuMemRecViewer(TestDirCRBaseModel):

    def get_op_mem_rec_dto(self, op_mem_rec: list) -> list:
        op_mem_rec_dto = []
        for data in op_mem_rec:
            data_dto = NpuOpMemRecDto()
            data_dto.component = data[0]
            data_dto.timestamp = data[1]
            data_dto.total_reserve_memory = data[2]
            data_dto.total_allocate_memory = data[3]
            data_dto.device_type = data[4]
            op_mem_rec_dto.append(data_dto)
        return op_mem_rec_dto

    def get_module_mem_dto(self, module_mem: list) -> list:
        module_mem_dto = []
        for data in module_mem:
            data_dto = NpuModuleMemDto()
            data_dto.module_id = data[0]
            data_dto.syscnt = data[1]
            data_dto.total_size = data[2]
            data_dto.device_type = data[3]
            module_mem_dto.append(data_dto)
        return module_mem_dto

    def test_get_summary_data_should_return_empty_when_model_init_fail(self):
        config = {"headers": ["component", "timestamp", "total_reserve_memory", "total_allocate_memory", "device_type"]}
        params = {
            "project": "test_npu_mem_rec_view",
            "model_id": 1,
            "iter_id": 1
        }
        check = NpuMemRecViewer(config, params)
        ret = check.get_summary_data()
        self.assertEqual(MsvpConstant.MSVP_EMPTY_DATA, ret)

    def test_get_summary_data_should_return_empty_when_invalid_module_id_is_included(self):
        config = {"headers": ["component", "timestamp", "total_reserve_memory", "total_allocate_memory", "device_type"]}
        params = {
            "project": "test_npu_module_mem_view",
            "model_id": 1,
            "iter_id": 1
        }
        InfoConfReader()._info_json = {
            'CPU': [{'Frequency': "1000"}]
        }
        InfoConfReader()._local_time_offset = 10.0
        invalid_module_mem_data = [
            [72, 0, 4096, 'NPU:0']
        ]
        invalid_module_mem_dto = self.get_module_mem_dto(invalid_module_mem_data)
        with mock.patch(NAMESPACE + '.NpuAiStackMemModel.check_db', side_effect=[False, True]), \
                mock.patch(NAMESPACE + '.NpuAiStackMemModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + '.NpuAiStackMemModel.get_table_data', return_value=invalid_module_mem_dto):
            check = NpuMemRecViewer(config, params)
            ret = check.get_summary_data()
            self.assertEqual(ret, MsvpConstant.MSVP_EMPTY_DATA)

    def test_get_summary_data_should_return_only_op_men_data_when_only_get_op_mem_rec_data(self):
        config = {"headers": ["component", "timestamp", "total_reserve_memory", "total_allocate_memory", "device_type"]}
        params = {
            "project": "test_npu_op_mem_view",
            "model_id": 1,
            "iter_id": 1
        }
        expected_headers = ['component', 'timestamp', 'total_reserve_memory', 'total_allocate_memory', 'device_type']
        expected_data = [
            ['GE', 10.0, 0.0, 0.0, 'NPU:5'],
            ['GE', 11.0, 1.0, 1.0, 'NPU:5']
        ]
        InfoConfReader()._info_json = {
            'CPU': [{'Frequency': "1000"}]
        }
        InfoConfReader()._local_time_offset = 10.0
        op_mem_rec_data = [
            ['GE', 0, 0, 0, 'NPU:5'],
            ['GE', 1000, 1024, 1024, 'NPU:5']
        ]
        op_mem_rec_dto = self.get_op_mem_rec_dto(op_mem_rec_data)
        with mock.patch(NAMESPACE + '.NpuAiStackMemModel.check_db', side_effect=[True, False]), \
                mock.patch(NAMESPACE + '.NpuAiStackMemModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + '.NpuAiStackMemModel.get_table_data', return_value=op_mem_rec_dto):
            check = NpuMemRecViewer(config, params)
            headers, data, _ = check.get_summary_data()
            self.assertEqual(headers, expected_headers)
            self.assertEqual(data, expected_data)

    def test_get_summary_data_should_return_only_module_men_data_when_only_get_module_mem_data(self):
        config = {"headers": ["component", "timestamp", "total_reserve_memory", "total_allocate_memory", "device_type"]}
        params = {
            "project": "test_npu_module_mem_view",
            "model_id": 1,
            "iter_id": 1
        }
        expected_headers = ['component', 'timestamp', 'total_reserve_memory', 'total_allocate_memory', 'device_type']
        expected_data = [
            ['RUNTIME', 10.0, None, 4096, 'NPU:0'],
            ['HDC', 11.0, None, 2048, 'NPU:0']
        ]
        InfoConfReader()._info_json = {
            'CPU': [{'Frequency': "1000"}]
        }
        InfoConfReader()._local_time_offset = 10.0
        module_mem_data = [
            [7, 0, 4096, 'NPU:0'],
            [9, 1000, 2048, 'NPU:0']
        ]
        module_mem_dto = self.get_module_mem_dto(module_mem_data)
        with mock.patch(NAMESPACE + '.NpuAiStackMemModel.check_db', side_effect=[False, True]), \
                mock.patch(NAMESPACE + '.NpuAiStackMemModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + '.NpuAiStackMemModel.get_table_data', return_value=module_mem_dto):
            check = NpuMemRecViewer(config, params)
            headers, data, _ = check.get_summary_data()
            self.assertEqual(headers, expected_headers)
            self.assertEqual(data, expected_data)

    def test_get_summary_data_should_return_both_module_men_data_and_op_rec_data_when_get_both_data(self):
        config = {"headers": ["component", "timestamp", "total_reserve_memory", "total_allocate_memory", "device_type"]}
        params = {
            "project": "test_npu_module_mem_view",
            "model_id": 1,
            "iter_id": 1
        }
        expected_headers = ['component', 'timestamp', 'total_reserve_memory', 'total_allocate_memory', 'device_type']
        expected_data = [
            ['GE', 10.0, 0.0, 0.0, 'NPU:0'],
            ['GE', 11.0, 1.0, 1.0, 'NPU:0'],
            ['RUNTIME', 10.0, None, 4096, 'NPU:0'],
            ['HDC', 11.0, None, 2048, 'NPU:0']
        ]
        InfoConfReader()._info_json = {
            'CPU': [{'Frequency': "1000"}]
        }
        InfoConfReader()._local_time_offset = 10.0
        op_mem_rec_data = [
            ['GE', 0, 0, 0, 'NPU:0'],
            ['GE', 1000, 1024, 1024, 'NPU:0']
        ]
        op_mem_rec_dto = self.get_op_mem_rec_dto(op_mem_rec_data)
        module_mem_data = [
            [7, 0, 4096, 'NPU:0'],
            [9, 1000, 2048, 'NPU:0']
        ]
        module_mem_dto = self.get_module_mem_dto(module_mem_data)
        with mock.patch(NAMESPACE + '.NpuAiStackMemModel.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.NpuAiStackMemModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + '.NpuAiStackMemModel.get_table_data',
                           side_effect=[op_mem_rec_dto, module_mem_dto]):
            check = NpuMemRecViewer(config, params)
            headers, data, _ = check.get_summary_data()
            self.assertEqual(headers, expected_headers)
            self.assertEqual(data, expected_data)

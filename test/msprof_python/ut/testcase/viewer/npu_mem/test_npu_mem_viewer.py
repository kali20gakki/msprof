#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from common_func.msvp_constant import MsvpConstant
from msparser.npu_mem.npu_mem_dto import NpuMemDto
from viewer.npu_mem.npu_mem_viewer import NpuMemViewer

NAMESPACE = 'viewer.npu_mem.npu_mem_viewer'


class TestNpuMemViewer(unittest.TestCase):

    def test_get_summary_data_should_return_empty_when_model_init_fail(self):
        config = {"headers": ["event", "ddr", "hbm", "timestamp", "memory"]}
        params = {
            "project": "test_npu_mem_view",
            "model_id": 1,
            "iter_id": 1
        }
        check = NpuMemViewer(config, params)
        ret = check.get_summary_data()
        self.assertEqual(MsvpConstant.MSVP_EMPTY_DATA, ret)

    def test_get_summary_data_should_return_success_when_model_init_ok(self):
        config = {"headers": ["event", "ddr", "hbm", "timestamp", "memory"]}
        params = {
            "project": "test_npu_mem_view",
            "model_id": 1,
            "iter_id": 1
        }
        npu_mem_dto = NpuMemDto()
        npu_mem_dto.event = '1'
        npu_mem_dto.hbm = 0
        npu_mem_dto.ddr = 0
        npu_mem_dto.memory = 0
        npu_mem_dto.timestamp = 5
        with mock.patch(NAMESPACE + '.NpuMemModel.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.NpuMemModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + '.NpuMemModel.get_summary_data', return_value=[npu_mem_dto]):
            check = NpuMemViewer(config, params)
            ret = check.get_summary_data()
            self.assertEqual((["event", "ddr", "hbm", "timestamp", "memory"],
                              [['Device', 0.0, 0.0, 0.0, 5]],
                              1), ret)

    def test_get_timeline_data_should_return_empty_when_db_check_fail(self):
        config = {"headers": ["event", "ddr", "hbm", "timestamp", "memory"]}
        params = {
            "project": "test_npu_mem_view",
            "model_id": 1,
            "iter_id": 1
        }
        check = NpuMemViewer(config, params)
        ret = check.get_timeline_data()
        self.assertEqual('{"status": 1, '
                         '"info": "Failed to connect npu_mem.db"}', ret)

    def test_get_timeline_data_should_return_empty_when_db_check_ok(self):
        config = {"headers": ["event", "ddr", "hbm", "timestamp", "memory"]}
        params = {
            "project": "test_npu_mem_view",
            "model_id": 1,
            "iter_id": 1
        }
        with mock.patch(NAMESPACE + '.NpuMemModel.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.NpuMemModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + '.NpuMemModel.get_timeline_data', return_value=[]):
            check = NpuMemViewer(config, params)
            ret = check.get_timeline_data()
            self.assertEqual('{"status": 1, '
                             '"info": "Unable to get npu mem data."}', ret)

    def test_get_timeline_data_should_return_empty_when_data_exist(self):
        InfoConfReader()._info_json = {"devices": '0'}
        config = {"headers": ["event", "ddr", "hbm", "timestamp", "memory"]}
        params = {
            "project": "test_npu_mem_view",
            "model_id": 1,
            "iter_id": 1
        }
        npu_mem_dto = NpuMemDto()
        npu_mem_dto.event = '0'
        npu_mem_dto.hbm = 0
        npu_mem_dto.ddr = 0
        npu_mem_dto.memory = 0
        npu_mem_dto.timestamp = 6
        with mock.patch(NAMESPACE + '.NpuMemModel.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.NpuMemModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + '.NpuMemModel.get_timeline_data', return_value=[npu_mem_dto]):
            check = NpuMemViewer(config, params)
            ret = check.get_timeline_data()
            self.assertEqual('[{"name": "process_name", "pid": 0, "tid": 0, "args": {"name": "NPU_MEM"}, '
                             '"ph": "M"}, {"name": "APP/DDR", "ts": 6.0, "pid": 0, "tid": 0, "args": '
                             '{"KB": 0.0}, "ph": "C"}, {"name": "APP/HBM", "ts": 6.0, "pid": 0, "tid": 0, '
                             '"args": {"KB": 0.0}, "ph": "C"}, {"name": "APP/Memory", "ts": 6.0, "pid": 0, '
                             '"tid": 0, "args": {"KB": 0.0}, "ph": "C"}]', ret)

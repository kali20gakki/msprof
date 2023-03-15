#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

import unittest
from unittest import mock

from msmodel.npu_mem.npu_mem_model import NpuMemModel
from common_func.info_conf_reader import InfoConfReader

NAMESPACE = 'msmodel.npu_mem.npu_mem_model'


class TestNpuMemModel(unittest.TestCase):

    def test_flush(self):
        with mock.patch(NAMESPACE + ".NpuMemModel.check_db"), \
                mock.patch(NAMESPACE + ".NpuMemModel.check_table"), \
                mock.patch(NAMESPACE + ".NpuMemModel.create_table"), \
                mock.patch(NAMESPACE + ".NpuMemModel.insert_data_to_db"):
            check = NpuMemModel("test", "npu_mem.db", ["NpuMem"])
            check.flush([])

    def test_get_timeline_data(self):
        with mock.patch(NAMESPACE + ".DBManager.fetch_all_data"):
            check = NpuMemModel("test", "npu_mem.db", ["NpuMem"])
            check.get_timeline_data()

    def test_get_summary_data(self):
        with mock.patch(NAMESPACE + ".DBManager.fetch_all_data"):
            check = NpuMemModel("test", "npu_mem.db", ["NpuMem"])
            check.get_summary_data()

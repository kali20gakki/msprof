#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

import unittest
from unittest import mock

from msmodel.npu_mem.npu_op_mem_model import NpuOpMemModel

NAMESPACE = 'msmodel.npu_mem.npu_op_mem_model'


class TestNpuOpMemModel(unittest.TestCase):

    def test_flush(self):
        with mock.patch(NAMESPACE + ".NpuOpMemModel.check_db"), \
                mock.patch(NAMESPACE + ".NpuOpMemModel.check_table"), \
                mock.patch(NAMESPACE + ".NpuOpMemModel.create_table"), \
                mock.patch(NAMESPACE + ".NpuOpMemModel.insert_data_to_db"):
            check = NpuOpMemModel("test", ["NpuOpMem"])
            table_name = "NpuOpMem"
            check.flush(table_name, [])

    def test_get_table_data(self):
        with mock.patch(NAMESPACE + ".DBManager.fetch_all_data"):
            check = NpuOpMemModel("test", ["NpuOpMem"])
            table_name = "NpuOpMem"
            check.get_table_data(table_name)

    def test_get_summary_data(self):
        with mock.patch(NAMESPACE + ".DBManager.fetch_all_data"):
            check = NpuOpMemModel("test", ["NpuOpMem"])
            table_name = "NpuOpMem"
            check.get_summary_data(table_name)

    def test_clear(self):
        with mock.patch(NAMESPACE + ".PathManager.get_db_path"), \
                mock.patch(NAMESPACE + ".DBManager.check_tables_in_db"), \
                mock.patch(NAMESPACE + ".DBManager.drop_table"):
            check = NpuOpMemModel("test", ["NpuOpMem"])
            table_name = "NpuOpMem"
            check.clear(table_name)

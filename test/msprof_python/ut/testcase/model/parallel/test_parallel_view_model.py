#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.

import unittest
from unittest import mock

from common_func.db_name_constant import DBNameConstant
from msmodel.parallel.parallel_model import ParallelViewModel

NAMESPACE = "msmodel.parallel.parallel_model"


class TestParallelViewModel(unittest.TestCase):
    def test_get_parallel_index_data_when_data_parallel(self):
        with mock.patch(NAMESPACE + ".DBManager.fetch_all_data", return_value=[[1]]):
            with ParallelViewModel("test") as _model:
                self.assertEqual(_model.get_parallel_index_data(DBNameConstant.TABLE_CLUSTER_DATA_PARALLEL, 1, 1, 1),
                                 [[1]])

    def test_get_parallel_index_data_when_model_parallel(self):
        with mock.patch(NAMESPACE + ".DBManager.fetch_all_data", return_value=[[2]]):
            with ParallelViewModel("test") as _model:
                self.assertEqual(_model.get_parallel_index_data(DBNameConstant.TABLE_CLUSTER_MODEL_PARALLEL, 1, 1, 1),
                                 [[2]])

    def test_get_parallel_index_data_when_pipeline_parallel(self):
        with mock.patch(NAMESPACE + ".DBManager.fetch_all_data", return_value=[[3]]):
            with ParallelViewModel("test") as _model:
                self.assertEqual(
                    _model.get_parallel_index_data(DBNameConstant.TABLE_CLUSTER_PIPELINE_PARALLEL, 1, 1, 1), [[3]])

    def test_get_parallel_strategy_data(self):
        with mock.patch(NAMESPACE + ".DBManager.fetch_all_data", return_value=[[4]]):
            with ParallelViewModel("test") as _model:
                self.assertEqual(_model.get_parallel_strategy_data(), [[4]])

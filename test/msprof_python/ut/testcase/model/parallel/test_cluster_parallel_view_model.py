#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.

import unittest
from unittest import mock

from msmodel.parallel.cluster_parallel_model import ClusterParallelViewModel

NAMESPACE = "msmodel.parallel.cluster_parallel_model"


class TestClusterParallelViewModel(unittest.TestCase):

    def test_get_npu_ids_1(self):
        with mock.patch(NAMESPACE + ".DBManager.fetch_all_data", return_value=[]):
            with ClusterParallelViewModel("test") as _model:
                self.assertEqual(_model.get_npu_ids("test"), [])

    def test_get_npu_ids_2(self):
        with mock.patch(NAMESPACE + ".DBManager.fetch_all_data", return_value=[[1]]):
            with ClusterParallelViewModel("test") as _model:
                self.assertEqual(_model.get_npu_ids("test"), [1])

    def test_get_model_iteration_ids_1(self):
        with mock.patch(NAMESPACE + ".DBManager.fetch_all_data", return_value=[]):
            with ClusterParallelViewModel("test") as _model:
                self.assertEqual(_model.get_model_iteration_ids("test"), {})

    def test_get_model_iteration_ids_2(self):
        with mock.patch(NAMESPACE + ".DBManager.fetch_all_data", return_value=[[5, '1,3,4']]):
            with ClusterParallelViewModel("test") as _model:
                self.assertEqual(_model.get_model_iteration_ids("test"), {5: ['1', '3', '4']})

    def test_get_table_name_1(self):
        with mock.patch(NAMESPACE + ".DBManager.fetch_all_data", return_value=[]):
            with ClusterParallelViewModel("test") as _model:
                self.assertEqual(_model.get_table_name(), 'N/A')

    def test_get_table_name_2(self):
        with mock.patch(NAMESPACE + ".DBManager.fetch_all_data", return_value=[['cluster']]):
            with ClusterParallelViewModel("test") as _model:
                self.assertEqual(_model.get_table_name(), 'cluster')

    def test_get_parallel_type_1(self):
        with mock.patch(NAMESPACE + ".DBManager.fetch_all_data", return_value=[]):
            with ClusterParallelViewModel("test") as _model:
                self.assertEqual(_model.get_parallel_type(), 'N/A')

    def test_get_parallel_type_2(self):
        with mock.patch(NAMESPACE + ".DBManager.fetch_all_data", return_value=[['cluster']]):
            with ClusterParallelViewModel("test") as _model:
                self.assertEqual(_model.get_parallel_type(), 'cluster')

    def test_get_first_field_name_1(self):
        params = {"npu_id": -1}
        with mock.patch(NAMESPACE + ".DBManager.fetch_all_data", return_value=[[1, 2]]):
            with ClusterParallelViewModel("test") as _model:
                self.assertEqual(_model.get_first_field_name(params)[1], "Rank ID")
                self.assertEqual(_model.get_first_field_name(params)[0], "rank_id")

    def test_get_first_field_name_2(self):
        params = {"npu_id": 1}
        with ClusterParallelViewModel("test") as _model:
            self.assertEqual(_model.get_first_field_name(params)[1], "Iteration ID")
            self.assertEqual(_model.get_first_field_name(params)[0], "iteration_id")

    def test_get_parallel_condition_and_query_params_1(self):
        params = {"npu_id": -1, "model_id": 1, "iteration_id": 1}
        with mock.patch(NAMESPACE + ".DBManager.fetch_all_data", return_value=[[1, 2]]):
            with ClusterParallelViewModel("test") as _model:
                self.assertEqual(_model.get_parallel_condition_and_query_params(params),
                                 ["model_id=? and iteration_id=?", (1, 1)])

    def test_get_parallel_condition_and_query_params_2(self):
        params = {"npu_id": 1, "model_id": 1, "iteration_id": -1}
        with mock.patch(NAMESPACE + ".ClusterParallelViewModel._get_npu_id_name", return_value="rank_id"):
            with ClusterParallelViewModel("test") as _model:
                self.assertEqual(_model.get_parallel_condition_and_query_params(params),
                                 ["rank_id=? and model_id=?", (1, 1)])

    def test_get_parallel_data(self):
        with mock.patch(NAMESPACE + ".DBManager.fetch_all_data", return_value=[[1, 2, 3]]):
            with ClusterParallelViewModel("test") as _model:
                self.assertEqual(_model.get_data_parallel_data("rank", "test", (1, 1)), [[1, 2, 3]])
                self.assertEqual(_model.get_model_parallel_data("rank", "test", (1, 1)), [[1, 2, 3]])
                self.assertEqual(_model.get_pipeline_parallel_data("rank", "test", (1, 1)), [[1, 2, 3]])

    def test_get_parallel_tuning_data(self):
        with mock.patch(NAMESPACE + ".DBManager.fetch_all_data", return_value=[[1, 2, 3]]):
            with ClusterParallelViewModel("test") as _model:
                self.assertEqual(_model.get_data_parallel_tuning_data(), [[1, 2, 3]])
                self.assertEqual(_model.get_model_parallel_tuning_data(), [[1, 2, 3]])
                self.assertEqual(_model.get_pipeline_parallel_tuning_data(), [[1, 2, 3]])

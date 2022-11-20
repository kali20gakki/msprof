#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

import os
import unittest
from unittest import mock

from msparser.parallel.parallel_query.model_parallel_analysis import ModelParallelAnalysis

NAMESPACE = "msparser.parallel.parallel_query.model_parallel_analysis"


class TestModelParallelAnalysis(unittest.TestCase):
    DIR_PATH = os.path.join(os.path.dirname(__file__), 'DT_ModelParallelAnalysis')
    PARAMS1 = {"collection_path": DIR_PATH,
               "npu_id": -1,
               "model_id": 1,
               "iteration_id": 2}
    PARAMS2 = {"collection_path": DIR_PATH,
               "npu_id": -1,
               "model_id": -1,
               "iteration_id": -1}

    def test_get_parallel_data(self) -> None:
        with mock.patch(NAMESPACE + ".ClusterParallelViewModel.get_first_field_name", return_value=("rank_id", "R")), \
                mock.patch(NAMESPACE + ".ClusterParallelViewModel.get_parallel_condition"), \
                mock.patch(NAMESPACE + ".ClusterParallelViewModel.get_data_parallel_data", return_value=[]):
            check = ModelParallelAnalysis(self.PARAMS1)
            self.assertEqual(check.get_parallel_data().get("headers")[0], "R")

    def test_get_tuning_suggestion(self) -> None:
        with mock.patch(NAMESPACE + ".ClusterParallelViewModel.get_model_parallel_tuning_data",
                        return_value=[[0.01]]), \
                mock.patch(NAMESPACE + ".ClusterParallelViewModel.get_parallel_type", return_value="auto_parallel"):
            check = ModelParallelAnalysis(self.PARAMS2)
            self.assertEqual(not check.get_tuning_suggestion().get("suggestion"), False)

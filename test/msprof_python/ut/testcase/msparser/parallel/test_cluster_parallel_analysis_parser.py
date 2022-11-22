#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

import os
import unittest
from unittest import mock

import pytest

from common_func.constant import Constant
from common_func.msprof_exception import ProfException
from constant.constant import clear_dt_project
from msparser.parallel.parallel_query.cluster_parallel_analysis_parser import ClusterParallelAnalysisParser

NAMESPACE = "msparser.parallel.parallel_query.cluster_parallel_analysis_parser"


class TestClusterParallelAnalysisParser(unittest.TestCase):
    DIR_PATH = os.path.join(os.path.dirname(__file__), 'DT_ClusterParallelAnalysisParser')
    PARAMS1 = {"collection_path": DIR_PATH,
               "npu_id": -1,
               "model_id": 1,
               "iteration_id": 2}
    PARAMS2 = {"collection_path": DIR_PATH,
               "npu_id": 1,
               "model_id": 1,
               "iteration_id": -1}
    PARAMS3 = {"collection_path": DIR_PATH,
               "npu_id": 1,
               "model_id": 1,
               "iteration_id": 5}

    def setUp(self) -> None:
        os.makedirs(os.path.join(self.DIR_PATH, 'PROF1', 'device_0'))
        os.makedirs(os.path.join(self.DIR_PATH, 'sqlite'))

    def tearDown(self) -> None:
        clear_dt_project(self.DIR_PATH)

    def test_process_should_raise_exception_when_without_db(self) -> None:
        with pytest.raises(ProfException) as err:
            with mock.patch(NAMESPACE + ".os.path.exists", return_value=False):
                check = ClusterParallelAnalysisParser(self.PARAMS1)
                check.process()
            self.assertEqual(ProfException.PROF_CLUSTER_INVALID_DB, err.value.code)

    def test_process_should_raise_exception_when_without_table(self) -> None:
        with pytest.raises(ProfException) as err:
            with mock.patch(NAMESPACE + ".os.path.exists", return_value=True), \
                    mock.patch(NAMESPACE + ".ClusterParallelViewModel.get_table_name", return_value=Constant.NA):
                check = ClusterParallelAnalysisParser(self.PARAMS1)
                check.process()
            self.assertEqual(ProfException.PROF_CLUSTER_INVALID_DB, err.value.code)

    def test_process_should_raise_exception_when_invalid_model_id(self) -> None:
        with pytest.raises(ProfException) as err:
            with mock.patch(NAMESPACE + ".os.path.exists", return_value=True), \
                    mock.patch(NAMESPACE + ".ClusterParallelViewModel.get_table_name", return_value="ClusterDataPa"), \
                    mock.patch(NAMESPACE + ".ClusterParallelViewModel.get_npu_ids", return_value=[1, 2]), \
                    mock.patch(NAMESPACE + ".ClusterParallelViewModel.get_model_iteration_ids",
                               return_value={2: [1, 2]}):
                check = ClusterParallelAnalysisParser(self.PARAMS1)
                check.process()
            self.assertEqual(ProfException.PROF_INVALID_PARAM_ERROR, err.value.code)

    def test_process_should_raise_exception_when_invalid_iteration_id(self) -> None:
        with pytest.raises(ProfException) as err:
            with mock.patch(NAMESPACE + ".os.path.exists", return_value=True), \
                    mock.patch(NAMESPACE + ".ClusterParallelViewModel.get_table_name", return_value="ClusterDataPa"), \
                    mock.patch(NAMESPACE + ".ClusterParallelViewModel.get_npu_ids", return_value=[1, 2]), \
                    mock.patch(NAMESPACE + ".ClusterParallelViewModel.get_model_iteration_ids",
                               return_value={1: [3, 4]}):
                check = ClusterParallelAnalysisParser(self.PARAMS1)
                check.process()
            self.assertEqual(ProfException.PROF_INVALID_PARAM_ERROR, err.value.code)

    def test_process_should_raise_exception_when_invalid_npu_id(self) -> None:
        with pytest.raises(ProfException) as err:
            with mock.patch(NAMESPACE + ".os.path.exists", return_value=True), \
                    mock.patch(NAMESPACE + ".ClusterParallelViewModel.get_table_name", return_value="ClusterDataPa"), \
                    mock.patch(NAMESPACE + ".ClusterParallelViewModel.get_npu_ids", return_value=[2, 3]), \
                    mock.patch(NAMESPACE + ".ClusterParallelViewModel.get_model_iteration_ids",
                               return_value={1: [3, 4]}):
                check = ClusterParallelAnalysisParser(self.PARAMS2)
                check.process()
            self.assertEqual(ProfException.PROF_INVALID_PARAM_ERROR, err.value.code)

    def test_process_should_raise_exception_when_invalid_params(self) -> None:
        with pytest.raises(ProfException) as err:
            with mock.patch(NAMESPACE + ".os.path.exists", return_value=True), \
                    mock.patch(NAMESPACE + ".ClusterParallelViewModel.get_table_name", return_value="ClusterDataPa"), \
                    mock.patch(NAMESPACE + ".ClusterParallelViewModel.get_npu_ids", return_value=[1, 2]), \
                    mock.patch(NAMESPACE + ".ClusterParallelViewModel.get_model_iteration_ids",
                               return_value={1: [1, 2]}):
                check = ClusterParallelAnalysisParser(self.PARAMS3)
                check.process()
            self.assertEqual(ProfException.PROF_INVALID_PARAM_ERROR, err.value.code)

    def test_process(self):
        with mock.patch(NAMESPACE + ".ClusterParallelAnalysisParser._prepare_parallel_analysis"), \
                mock.patch(NAMESPACE + ".ClusterParallelAnalysis.get_parallel_data",
                           return_value={"parallel_mode": "Data Parallel"}):
            check = ClusterParallelAnalysisParser(self.PARAMS2)
            check.process()
            self.assertEqual(
                os.path.exists(os.path.join(self.DIR_PATH, "query", "cluster_parallel_analysis_1_1_-1.json")), True)

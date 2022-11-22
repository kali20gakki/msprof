#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

import os
import unittest
from argparse import Namespace
from unittest import mock

import pytest

from common_func.msprof_exception import ProfException
from constant.constant import clear_dt_project
from msinterface.msprof_query_summary_manager import MsprofQuerySummaryManager

NAMESPACE = 'msinterface.msprof_query_summary_manager'


class TestMsprofQuerySummaryManager(unittest.TestCase):
    DIR_PATH = os.path.join(os.path.dirname(__file__), 'DT_MsprofQuerySummaryManager')

    def setUp(self) -> None:
        os.makedirs(os.path.join(self.DIR_PATH, 'PROF1', 'devie_0'))

    def tearDown(self) -> None:
        clear_dt_project(self.DIR_PATH)

    def test_check_rank_id_should_return_false_when_prof_path_invalid(self):
        ret = MsprofQuerySummaryManager.check_rank_id(self.DIR_PATH)
        self.assertFalse(ret)

    def test_process_should_return_empty_when_data_type_invalid(self):
        args_dic = {"collection_path": self.DIR_PATH,
                    "id": 1,
                    "data_type": "aa",
                    "model_id": 1,
                    "iteration_id": -1}
        args = Namespace(**args_dic)
        with pytest.raises(ProfException) as err:
            check = MsprofQuerySummaryManager(args)
            check.process()
        self.assertEqual(ProfException.PROF_INVALID_PARAM_ERROR, err.value.code)

    def test_dispatch_should_return_empty_when_check_tables_failed(self):
        args_dic = {"collection_path": self.DIR_PATH,
                    "id": 1,
                    "data_type": 1,
                    "model_id": 1,
                    "iteration_id": 1}
        args = Namespace(**args_dic)
        with mock.patch("os.path.exists", return_value=True), \
                pytest.raises(ProfException) as err:
            check = MsprofQuerySummaryManager(args)
            check.process()
        self.assertEqual(ProfException.PROF_CLUSTER_DIR_ERROR, err.value.code)

    def test_dispatch_should_return_empty_when_data_type_valid_npu_id_invalid(self):
        args_dic = {"collection_path": self.DIR_PATH,
                    "id": -2,
                    "data_type": 1,
                    "model_id": 1,
                    "iteration_id": 1}
        args = Namespace(**args_dic)
        with mock.patch(NAMESPACE + ".MsprofQuerySummaryManager._check_collection_dir_valid", return_value=True), \
                pytest.raises(ProfException) as err:
            check = MsprofQuerySummaryManager(args)
            check.process()
        self.assertEqual(ProfException.PROF_INVALID_PARAM_ERROR, err.value.code)

    def test_dispatch_should_return_empty_when_data_type_valid_model_invalid(self):
        args_dic = {"collection_path": self.DIR_PATH,
                    "id": 1,
                    "data_type": 1,
                    "model_id": -1,
                    "iteration_id": 1}
        args = Namespace(**args_dic)
        with mock.patch(NAMESPACE + ".MsprofQuerySummaryManager._check_collection_dir_valid", return_value=True),\
                pytest.raises(ProfException) as err:
            check = MsprofQuerySummaryManager(args)
            check.process()
        self.assertEqual(ProfException.PROF_INVALID_PARAM_ERROR, err.value.code)

    def test_dispatch_should_return_empty_when_data_type_valid_iter_invalid(self):
        args_dic = {"collection_path": self.DIR_PATH,
                    "id": 1,
                    "data_type": 1,
                    "model_id": 1,
                    "iteration_id": -2}
        args = Namespace(**args_dic)
        with mock.patch(NAMESPACE + ".MsprofQuerySummaryManager._check_collection_dir_valid", return_value=True),\
                pytest.raises(ProfException) as err:
            check = MsprofQuerySummaryManager(args)
            check.process()
        self.assertEqual(ProfException.PROF_INVALID_PARAM_ERROR, err.value.code)

    def test_check_cluster_scene_should_return_empty_when_data_type_is_0(self):
        args_dic = {"collection_path": self.DIR_PATH,
                    "id": 1,
                    "data_type": 0,
                    "model_id": 1,
                    "iteration_id": -2}
        args = Namespace(**args_dic)
        check = MsprofQuerySummaryManager(args)
        check.process()

    def test_check_cluster_scene_should_return_empty_when_data_type_is_1(self):
        args_dic = {"collection_path": self.DIR_PATH,
                    "id": 1,
                    "data_type": 1,
                    "model_id": 1,
                    "iteration_id": 1}
        args = Namespace(**args_dic)
        with mock.patch(NAMESPACE + ".MsprofQuerySummaryManager._check_collection_dir_valid", return_value=True),\
                mock.patch(NAMESPACE + '.StepTraceSummay.process'):
            check = MsprofQuerySummaryManager(args)
            check.process()

    def test_check_cluster_scene_should_return_empty_when_data_type_is_2(self):
        args_dic = {"collection_path": self.DIR_PATH,
                    "id": 1,
                    "data_type": 2,
                    "model_id": 1,
                    "iteration_id": 1}
        args = Namespace(**args_dic)
        with mock.patch(NAMESPACE + ".MsprofQuerySummaryManager._check_collection_dir_valid", return_value=True),\
                mock.patch(NAMESPACE + '.FopsParser.process'):
            check = MsprofQuerySummaryManager(args)
            check.process()

    def test_check_cluster_scene_should_return_empty_when_data_type_is_5(self):
        args_dic = {"collection_path": self.DIR_PATH,
                    "id": 1,
                    "data_type": 5,
                    "model_id": 1,
                    "iteration_id": 1}
        args = Namespace(**args_dic)
        with mock.patch(NAMESPACE + ".MsprofQuerySummaryManager._check_collection_dir_valid", return_value=True),\
                mock.patch(NAMESPACE + '.ClusterParallelAnalysisParser.process'):
            check = MsprofQuerySummaryManager(args)
            check.process()


    def test_check_cluster_scene_should_return_empty_when_data_type_is_6(self):
        args_dic = {"collection_path": self.DIR_PATH,
                    "id": 1,
                    "data_type": 6,
                    "model_id": 1,
                    "iteration_id": 1}
        args = Namespace(**args_dic)
        with mock.patch(NAMESPACE + ".MsprofQuerySummaryManager._check_collection_dir_valid", return_value=True),\
                mock.patch(NAMESPACE + '.ClusterTuningFacade.process'):
            check = MsprofQuerySummaryManager(args)
            check.process()

    def test_check_cluster_scene_should_return_empty_when_data_type_is_7(self):
        args_dic = {"collection_path": self.DIR_PATH,
                    "id": 1,
                    "data_type": 7,
                    "model_id": 1,
                    "iteration_id": 1}
        args = Namespace(**args_dic)
        with mock.patch(NAMESPACE + ".MsprofQuerySummaryManager._check_collection_dir_valid", return_value=True),\
                mock.patch(NAMESPACE + '.ClusterTuningFacade.process'):
            check = MsprofQuerySummaryManager(args)
            check.process()
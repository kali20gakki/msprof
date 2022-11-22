#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

import os
import unittest
from unittest import mock
from tuning.cluster.cluster_tuning_facade import ClusterTuningFacade
from common_func.msprof_exception import ProfException
from common_func.ms_constant.str_constant import StrConstant
from constant.constant import clear_dt_project
import pytest


NAMESPACE = 'tuning.cluster.cluster_tuning_facade'


class TestClusterTuningFacade(unittest.TestCase):
    DIR = os.path.join(os.path.dirname(__file__), 'DT_ClusterTuningFacade')
    params = {"collection_path": os.path.join(os.path.dirname(__file__), 'DT_ClusterTuningFacade'),
              "npu_id": 1,
              "model_id": 0,
              "iteration_id": 1,
              "data_type": 6}

    def setUp(self) -> None:
        os.makedirs(self.params['collection_path'])

    def tearDown(self) -> None:
        clear_dt_project(self.params['collection_path'])

    def test_cluster_communication(self):
        ClusterTuningFacade(self.params).cluster_communication()

    def test_run(self):
        with mock.patch(NAMESPACE + '.ClusterTuningFacade._check_data_type_valid'), \
                mock.patch(NAMESPACE + '.ClusterTuningFacade._check_collection_dir_valid'), \
                mock.patch(NAMESPACE + '.ClusterTuningFacade.dispatch'):
            ClusterTuningFacade(self.params).process()

    def test_run_invalid_collection_prof1(self):
        with mock.patch(NAMESPACE + '.ClusterTuningFacade._check_data_type_valid'), \
             mock.patch(NAMESPACE + '.ClusterTuningFacade.dispatch'), \
             pytest.raises(ProfException) as err:
            ClusterTuningFacade(self.params).process()
            self.assertEqual(ProfException.PROF_CLUSTER_DIR_ERROR, err.value.code)

    def test_run_invalid_model_id_prof(self):
        with mock.patch(NAMESPACE + '.ClusterTuningFacade._check_data_type_valid'), \
                mock.patch(NAMESPACE + '.ClusterTuningFacade._check_collection_dir_valid'), \
                mock.patch(NAMESPACE + '.ClusterTuningFacade.dispatch'), \
                pytest.raises(ProfException) as err:
            params = {"collection_path": self.DIR,
                      "npu_id": 1,
                      "model_id": -1,
                      "iteration_id": 1,
                      "data_type": 6}
            ClusterTuningFacade(params).process()
            self.assertEqual(ProfException.PROF_INVALID_PARAM_ERROR, err.value.code)

    def test_run_model_id_is_none(self):
        with mock.patch(NAMESPACE + '.ClusterTuningFacade._check_data_type_valid'), \
                mock.patch(NAMESPACE + '.ClusterTuningFacade._check_collection_dir_valid'), \
                mock.patch(NAMESPACE + '.ClusterTuningFacade.dispatch'):
            params = {"collection_path": self.DIR,
                      "npu_id": 1,
                      "iteration_id": 1,
                      "data_type": 6}
            ClusterTuningFacade(params).process()

    def test_run_invalid_data_type_prof(self):
        with mock.patch(NAMESPACE + '.QueryArgumentCheck.check_arguments_valid'), \
                mock.patch(NAMESPACE + '.ClusterTuningFacade._check_collection_dir_valid'), \
                mock.patch(NAMESPACE + '.ClusterTuningFacade.dispatch'), \
                pytest.raises(ProfException) as err:
            params = {"collection_path": self.DIR,
                      "npu_id": 1,
                      "data_type": 122,
                      "model_id": 0,
                      "iteration_id": 1}
            ClusterTuningFacade(params).process()
            self.assertEqual(ProfException.PROF_INVALID_PARAM_ERROR, err.value.code)

    def test_run_data_type_is_none(self):
        with mock.patch(NAMESPACE + '.QueryArgumentCheck.check_arguments_valid'), \
                mock.patch(NAMESPACE + '.ClusterTuningFacade._check_collection_dir_valid'), \
                mock.patch(NAMESPACE + '.ClusterTuningFacade.dispatch'):
            params = {"collection_path": self.DIR,
                      "npu_id": 1,
                      "iteration_id": 1,
                      "model_id": 0}
            ClusterTuningFacade(params).process()

    def test_run_invalid_iteration_prof(self):
        with mock.patch(NAMESPACE + '.ClusterTuningFacade._check_collection_dir_valid'), \
                mock.patch(NAMESPACE + '.ClusterTuningFacade.dispatch'), \
                mock.patch(NAMESPACE + '.ClusterTuningFacade._check_data_type_valid'), \
                pytest.raises(ProfException) as err:
            params = {"collection_path": self.DIR,
                      "npu_id": 1,
                      "model_id": 0,
                      "iteration_id": -1,
                      "data_type": 6}
            ClusterTuningFacade(params).process()
        self.assertEqual(ProfException.PROF_INVALID_PARAM_ERROR, err.value.code)

    def test_run_invalid_iteration2_prof(self):
        with mock.patch(NAMESPACE + '.ClusterTuningFacade._check_collection_dir_valid'), \
                mock.patch(NAMESPACE + '.ClusterTuningFacade.dispatch'), \
                mock.patch(NAMESPACE + '.ClusterTuningFacade._check_data_type_valid'), \
                pytest.raises(ProfException) as err:
            params = {"collection_path": self.DIR,
                      "npu_id": 1,
                      "model_id": 0,
                      "data_type": 6}
            ClusterTuningFacade(params).process()
        self.assertEqual(ProfException.PROF_INVALID_PARAM_ERROR, err.value.code)


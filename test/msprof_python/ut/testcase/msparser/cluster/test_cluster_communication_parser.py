#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

import os
import unittest
from unittest import mock

from constant.constant import clear_dt_project
from msparser.cluster.cluster_communication_parser import ClusterCommunicationParser

NAMESPACE = 'msparser.cluster.cluster_communication_parser'


class TestClusterCommunicationParser(unittest.TestCase):
    DIR_PATH = os.path.join(os.path.dirname(__file__), 'DT_ClusterCommunicationParser')
    param = {
        "collection_path": DIR_PATH,
        "is_cluster": True,
        "npu_id": -1,
        "model_id": 1,
        "iteration_id": 1
    }

    def setUp(self) -> None:
        os.mkdir(self.DIR_PATH)

    def tearDown(self) -> None:
        clear_dt_project(self.DIR_PATH)

    def test_should_raise_empty_when_is_not_all_device_scene(self):
        case_param = {}
        case_param.update(self.param)
        case_param.update({"npu_id": 0})
        check = ClusterCommunicationParser(case_param)
        check.process()

    def test_should_raise_empty_when_cluster_step_trace_db_not_exist(self):
        case_param = {}
        case_param.update(self.param)
        check = ClusterCommunicationParser(case_param)
        check.process()

    def test_should_raise_empty_when_get_no_cluster_communication_data(self):
        case_param = {}
        case_param.update(self.param)
        with mock.patch('os.path.exists', return_value=True), \
                mock.patch('common_func.db_manager.DBManager.create_connect_db', return_value=(1, 1)), \
                mock.patch('common_func.db_manager.DBManager.judge_table_exist', return_value=True), \
                mock.patch(NAMESPACE + '.ClusterInfoViewModel.get_device_and_rank_ids',
                           return_value=[(1, 11), (2, 12), (3, 13)]), \
                mock.patch(NAMESPACE + '.ClusterCommunicationModel.get_cluster_communication', return_value=[]):
            check = ClusterCommunicationParser(case_param)
            check.process()

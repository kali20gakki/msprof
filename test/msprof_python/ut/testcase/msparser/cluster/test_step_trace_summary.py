#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

import os
import unittest
from unittest import mock

import pytest

from common_func.msprof_exception import ProfException
from constant.constant import clear_dt_project
from msparser.cluster.step_trace_summary import StepTraceSummay

NAMESPACE = 'msparser.cluster.step_trace_summary'


class TestStepTraceSummary(unittest.TestCase):
    DIR_PATH = os.path.join(os.path.dirname(__file__), 'DT_StepTraceSummary')
    param = {
        "collection_path": DIR_PATH,
        "is_cluster": True,
        "npu_id": 0,
        "model_id": 1,
        "iteration_id": 1
    }

    def setUp(self) -> None:
        os.mkdir(self.DIR_PATH)

    def tearDown(self) -> None:
        clear_dt_project(self.DIR_PATH)

    def test_should_raise_exception_when_npu_id_and_iter_id_is_no_cluster_scene(self):
        case_param = {}
        case_param.update(self.param)
        with pytest.raises(ProfException) as err:
            check = StepTraceSummay(case_param)
            check.process()
        self.assertEqual(ProfException.PROF_INVALID_PARAM_ERROR, err.value.code)

    def test_should_raise_exception_when_npu_id_and_iter_id_is_export_all(self):
        case_param = {}
        case_param.update(self.param)
        case_param.update({"npu_id": -1, "iteration_id": -1})
        with pytest.raises(ProfException) as err:
            check = StepTraceSummay(case_param)
            check.process()
        self.assertEqual(ProfException.PROF_INVALID_PARAM_ERROR, err.value.code)

    def test_should_cover_cluster_scene_when_npu_id_is_export_all_cluster_db_not_exist(self):
        case_param = {}
        case_param.update(self.param)
        case_param.update({"npu_id": -1})
        check = StepTraceSummay(case_param)
        check.process()

    def test_should_cover_cluster_scene_when_npu_id_is_export_all_cluster_rank_db_not_exist(self):
        case_param = {}
        case_param.update(self.param)
        case_param.update({"npu_id": -1})
        with mock.patch(NAMESPACE + '.StepTraceSummay._check_step_trace_db', return_value=True):
            check = StepTraceSummay(case_param)
            check.process()

    def test_should_cover_cluster_scene_when_npu_id_is_export_all_cluster_db_table_not_exist(self):
        case_param = {}
        case_param.update(self.param)
        case_param.update({"npu_id": -1})
        with mock.patch(NAMESPACE + '.StepTraceSummay._check_step_trace_db', return_value=True), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=False):
            check = StepTraceSummay(case_param)
            check.process()

    def test_should_cover_cluster_scene_when_npu_id_is_export_all_cluster_get_device_and_rank_ids(self):
        case_param = {}
        case_param.update(self.param)
        case_param.update({"npu_id": -1})
        with mock.patch(NAMESPACE + '.StepTraceSummay._check_step_trace_db', return_value=True), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True), \
                mock.patch(NAMESPACE + '.ClusterInfoViewModel.get_device_and_rank_ids',
                           return_value=[(1, 11), (2, 12), (3, 13)]):
            check = StepTraceSummay(case_param)
            check.process()

    def test_should_cover_cluster_scene_when_npu_id_is_export_all_cluster_get_no_device_and_rank_ids(self):
        case_param = {}
        case_param.update(self.param)
        case_param.update({"npu_id": -1})
        with mock.patch(NAMESPACE + '.StepTraceSummay._check_step_trace_db', return_value=True), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True), \
                mock.patch(NAMESPACE + '.ClusterInfoViewModel.get_device_and_rank_ids', return_value=[]):
            check = StepTraceSummay(case_param)
            check.process()

    def test_query_in_cluster_scene_should_cover_cluster_scene_when_npu_id_is_all_cluster_db_table_not_exist(self):
        case_param = {}
        case_param.update(self.param)
        case_param.update({"npu_id": -1})
        with mock.patch(NAMESPACE + '.StepTraceSummay._check_iteration_id_valid'), \
                mock.patch(NAMESPACE + '.StepTraceSummay._check_step_trace_db', return_value=True), \
                mock.patch(NAMESPACE + '.StepTraceSummay._get_rank_or_device_ids', return_value=set([1])):
            check = StepTraceSummay(case_param)
            check.process()

    def test_query_in_cluster_scene_should_cover_cluster_scene_when_npu_id_is_all_query_no_db_data(self):
        case_param = {}
        case_param.update(self.param)
        case_param.update({"npu_id": -1})
        with mock.patch(NAMESPACE + '.StepTraceSummay._check_iteration_id_valid'), \
                mock.patch(NAMESPACE + '.StepTraceSummay._check_step_trace_db', return_value=True), \
                mock.patch(NAMESPACE + '.StepTraceSummay._get_rank_or_device_ids', return_value=set([1])), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]):
            check = StepTraceSummay(case_param)
            check.process()

    def test_query_in_cluster_scene_should_cover_cluster_scene_when_npu_id_is_all(self):
        case_param = {}
        case_param.update(self.param)
        case_param.update({"npu_id": -1})
        with mock.patch(NAMESPACE + '.StepTraceSummay._check_iteration_id_valid'), \
                mock.patch(NAMESPACE + '.StepTraceSummay._check_step_trace_db', return_value=True), \
                mock.patch(NAMESPACE + '.StepTraceSummay._get_rank_or_device_ids', return_value=set([1])), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch(NAMESPACE + '.ClusterStepTraceViewModel.get_sql_data', return_value=[[1]]), \
                mock.patch(NAMESPACE + '.StepTraceSummay._storage_summary_data'):
            check = StepTraceSummay(case_param)
            check.process()

    def test_query_in_cluster_scene_should_cover_cluster_scene_when_iteration_id_is_all(self):
        case_param = {}
        case_param.update(self.param)
        case_param.update({"iteration_id": None})
        with mock.patch(NAMESPACE + '.StepTraceSummay._check_iteration_id_valid'), \
                mock.patch(NAMESPACE + '.StepTraceSummay._check_step_trace_db', return_value=True), \
                mock.patch(NAMESPACE + '.StepTraceSummay._get_rank_or_device_ids', return_value=set([1])), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]):
            check = StepTraceSummay(case_param)
            check.process()

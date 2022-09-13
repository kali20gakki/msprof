#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2022. All rights reserved.
"""
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

    def test_should_cover_cluster_scene_when_npu_id_is_export_all_cluster_db_no_exit(self):
        case_param = {}
        case_param.update(self.param)
        case_param.update({"npu_id": -1})
        check = StepTraceSummay(case_param)
        check.process()

    def test_should_cover_cluster_scene_when_npu_id_is_export_all_cluster_db_connect_not_exist(self):
        case_param = {}
        case_param.update(self.param)
        case_param.update({"npu_id": -1})
        with mock.patch(NAMESPACE + '.StepTraceSummay._check_cluster_db_valid', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(None, None)):
            check = StepTraceSummay(case_param)
            check.process()

    def test_should_cover_cluster_scene_when_npu_id_is_export_all_cluster_db_table_not_exist(self):
        case_param = {}
        case_param.update(self.param)
        case_param.update({"npu_id": -1})
        with mock.patch(NAMESPACE + '.StepTraceSummay._check_cluster_db_valid', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(1, 1)), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            check = StepTraceSummay(case_param)
            check.process()

    def test_should_cover_cluster_scene_when_npu_id_is_export_all_with_rank_ids(self):
        case_param = {}
        case_param.update(self.param)
        case_param.update({"npu_id": -1})
        with mock.patch(NAMESPACE + '.StepTraceSummay._check_cluster_db_valid', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(1, 1)), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[(0,)]), \
                mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'), \
                mock.patch(NAMESPACE + '.StepTraceSummay._query_in_cluster_scene'):
            check = StepTraceSummay(case_param)
            check.process()

    def test_query_in_cluster_scene_should_cover_cluster_scene_when_npu_id_is_all_cluster_db_table_not_exist(self):
        case_param = {}
        case_param.update(self.param)
        case_param.update({"npu_id": -1})
        with mock.patch(NAMESPACE + '.StepTraceSummay._check_iteration_id_valid'), \
                mock.patch(NAMESPACE + '.StepTraceSummay._check_cluster_db_valid', return_value=True), \
                mock.patch(NAMESPACE + '.StepTraceSummay._get_all_rank_ids', return_value=[1]), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(None, None)):
            check = StepTraceSummay(case_param)
            check.process()

    def test_query_in_cluster_scene_should_cover_cluster_scene_when_npu_id_is_all(self):
        case_param = {}
        case_param.update(self.param)
        case_param.update({"npu_id": -1})
        with mock.patch(NAMESPACE + '.StepTraceSummay._check_iteration_id_valid'), \
                mock.patch(NAMESPACE + '.StepTraceSummay._check_cluster_db_valid', return_value=True), \
                mock.patch(NAMESPACE + '.StepTraceSummay._get_all_rank_ids', return_value=[1]), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(1, 1)), \
                mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]):
            check = StepTraceSummay(case_param)
            check.process()

    def test_query_in_cluster_scene_should_cover_cluster_scene_when_iteration_id_is_all(self):
        case_param = {}
        case_param.update(self.param)
        case_param.update({"iteration_id": -1})
        with mock.patch(NAMESPACE + '.StepTraceSummay._check_iteration_id_valid'), \
                mock.patch(NAMESPACE + '.StepTraceSummay._check_cluster_db_valid', return_value=True), \
                mock.patch(NAMESPACE + '.StepTraceSummay._get_all_rank_ids', return_value=[1]), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(1, 1)), \
                mock.patch(NAMESPACE + '.QueryArgumentCheck.check_arguments_valid'), \
                mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'), \
                mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]):
            check = StepTraceSummay(case_param)
            check.process()

    def test_process_in_non_cluster_scene_should_is_no_cluster_scene_when_npu_id_is_all_without_prof_dirs(self):
        case_param = {}
        case_param.update(self.param)
        case_param.update({"npu_id": -1, "is_cluster": False})
        with mock.patch(NAMESPACE + '.StepTraceSummay._check_iteration_id_valid'), \
                mock.patch(NAMESPACE + '.get_path_dir', return_value=[]):
            check = StepTraceSummay(case_param)
            check.process()

    def test_process_in_non_cluster_scene_should_is_no_cluster_scene_when_npu_id_is_all(self):
        case_param = {}
        case_param.update(self.param)
        case_param.update({"npu_id": -1, "is_cluster": False})
        with mock.patch(NAMESPACE + '.StepTraceSummay._check_iteration_id_valid'), \
                mock.patch('os.path.isdir', return_value=True), \
                mock.patch(NAMESPACE + '.get_path_dir',
                           return_value=[os.path.join(self.DIR_PATH, 'PROF1')]), \
                mock.patch('os.listdir',
                           return_value=[os.path.join(self.DIR_PATH, 'PROF1', 'device_0')]), \
                mock.patch(NAMESPACE + '.DataCheckManager.contain_info_json_data', return_value=True):
            check = StepTraceSummay(case_param)
            check.process()

    def test_process_in_non_cluster_scene_should_is_no_cluster_scene_when_npu_id_is_all_db_trace_not_exist(self):
        case_param = {}
        case_param.update(self.param)
        case_param.update({"npu_id": -1, "is_cluster": False})
        with mock.patch(NAMESPACE + '.StepTraceSummay._check_iteration_id_valid'), \
                mock.patch(NAMESPACE + '.StepTraceSummay._find_info_file_dir',
                           return_value=self.DIR_PATH), \
                mock.patch(NAMESPACE + '.prepare_log'), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(None, None)):
            check = StepTraceSummay(case_param)
            check.process()

    def test_process_in_non_cluster_scene_should_is_no_cluster_scene_when_npu_id_is_all_table_trace_not_exist(self):
        case_param = {}
        case_param.update(self.param)
        case_param.update({"npu_id": -1, "is_cluster": False})
        with mock.patch(NAMESPACE + '.StepTraceSummay._check_iteration_id_valid'), \
                mock.patch(NAMESPACE + '.StepTraceSummay._find_info_file_dir',
                           return_value=self.DIR_PATH), \
                mock.patch(NAMESPACE + '.prepare_log'), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(1, 1)), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            check = StepTraceSummay(case_param)
            check.process()

    def test_process_in_non_cluster_scene_should_is_no_cluster_scene_when_npu_id_is_all_2(self):
        case_param = {}
        case_param.update(self.param)
        case_param.update({"npu_id": -1, "is_cluster": False})
        with mock.patch(NAMESPACE + '.StepTraceSummay._check_iteration_id_valid'), \
                mock.patch(NAMESPACE + '.StepTraceSummay._find_info_file_dir',
                           return_value=self.DIR_PATH), \
                mock.patch(NAMESPACE + '.prepare_log'), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(1, 1)), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]), \
                mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'), \
                mock.patch(NAMESPACE + '.StepTraceConstant.syscnt_to_micro'):
            check = StepTraceSummay(case_param)
            check.process()

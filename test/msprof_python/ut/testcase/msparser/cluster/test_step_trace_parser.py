#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2022. All rights reserved.
"""
import os
import unittest
from unittest import mock

from constant.constant import clear_dt_project
from msparser.cluster.cluster_step_trace_parser import ClusterStepTraceParser

NAMESPACE = 'msparser.cluster.cluster_step_trace_parser'


class TestStepTraceParser(unittest.TestCase):
    DIR_PATH = os.path.join(os.path.dirname(__file__), 'DT_StepTraceParser')

    def setUp(self) -> None:
        os.makedirs(os.path.join(self.DIR_PATH, 'PROF1', 'device_0'))

    def tearDown(self) -> None:
        clear_dt_project(self.DIR_PATH)

    def test_ms_run_should_return_empty_when_path_invalid(self):
        check = ClusterStepTraceParser('DT_StepTraceParser')
        check.ms_run()

    def test_ms_run_should_return_empty_when_path_valid_db_not_exist(self):
        with mock.patch(NAMESPACE + '.ClusterStepTraceParser._check_collection_path_valid', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(None, None)):
            check = ClusterStepTraceParser(self.DIR_PATH)
            check.ms_run()

    def test_ms_run_should_return_empty_when_path_valid_table_not_exist(self):
        with mock.patch(NAMESPACE + '.ClusterStepTraceParser._check_collection_path_valid', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(1, 1)), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False), \
                mock.patch(NAMESPACE + '.ClusterStepTraceParser.save'):
            check = ClusterStepTraceParser(self.DIR_PATH)
            check.ms_run()

    def test_ms_run_should_return_empty_when_data_exist(self):
        with mock.patch(NAMESPACE + '.ClusterStepTraceParser._check_collection_path_valid', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(1, 1)), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[(1, "PROF1/device_0")]), \
                mock.patch(NAMESPACE + '.DataCheckManager.contain_info_json_data', return_value=True):
            check = ClusterStepTraceParser(self.DIR_PATH)
            check.ms_run()

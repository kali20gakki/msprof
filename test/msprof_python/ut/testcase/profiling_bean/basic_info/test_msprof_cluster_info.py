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
from profiling_bean.basic_info.msprof_cluster_info import MsProfClusterInfo
from profiling_bean.db_dto.cluster_rank_dto import ClusterRankDto

NAMESPACE = 'profiling_bean.basic_info.msprof_cluster_info'


class TestMsProfClusterInfo(unittest.TestCase):
    DIR_PATH = os.path.join(os.path.dirname(__file__), 'DT_MsProfClusterInfo')

    def setUp(self) -> None:
        os.mkdir(self.DIR_PATH)

    def tearDown(self) -> None:
        clear_dt_project(self.DIR_PATH)

    def test_collect_cluster_info_data_should_return_empty_when_without_cluster_data(self):
        with mock.patch(NAMESPACE + '.ClusterInfoViewModel.check_table', return_value=False), \
                mock.patch(NAMESPACE + '.ClusterInfoViewModel.get_all_cluster_rank_info', return_value=[]):
            check = MsProfClusterInfo(self.DIR_PATH)
            check.run()

    def test_collect_cluster_info_data_should_return_empty_when_cluster_data_exist(self):
        cluster_rank_dto = ClusterRankDto()
        cluster_rank_dto.rank_id = 0
        cluster_rank_dto.device_id = 0
        cluster_rank_dto.job_info = "NA"
        cluster_rank_dto.dir_name = 'PROF1/device_0'
        with mock.patch(NAMESPACE + '.ClusterInfoViewModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + '.ClusterInfoViewModel.get_all_cluster_rank_info',
                           return_value=[cluster_rank_dto]), \
                mock.patch(NAMESPACE + '.ClusterStepTraceViewModel.judge_table_exist', return_value=True), \
                mock.patch(NAMESPACE + '.ClusterStepTraceViewModel.get_sql_data', return_value=[(1, 2)]):
            check = MsProfClusterInfo(self.DIR_PATH)
            check.run()

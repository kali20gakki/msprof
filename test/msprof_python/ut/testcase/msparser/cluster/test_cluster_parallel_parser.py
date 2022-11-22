#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

import os
import unittest
from unittest import mock

from common_func.db_manager import DBManager
from constant.constant import clear_dt_project
from msmodel.parallel.parallel_model import ParallelViewModel
from msparser.cluster.cluster_parallel_parser import ClusterParallelParser
from profiling_bean.db_dto.hccl_operator_dto import HCCLOperatorDto
from profiling_bean.db_dto.time_section_dto import TimeSectionDto
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = "msparser.cluster.cluster_parallel_parser"


class TestClusterParallelParser(unittest.TestCase):
    FILE_LIST_1 = {1: ["test"]}
    FILE_LIST_2 = {DataTag.PARALLEL_STRATEGY: ["test"]}
    DIR_PATH = os.path.join(os.path.dirname(__file__), 'DT_ClusterParallelParser')
    SAMPLE_CONFIG = {"result_dir": DIR_PATH}
    op_obj = HCCLOperatorDto()
    iter_obj = TimeSectionDto()
    SIDE_EFFECT = [[op_obj], [iter_obj]]

    def setUp(self) -> None:
        os.makedirs(os.path.join(self.DIR_PATH, 'PROF1', 'device_0'))
        os.makedirs(os.path.join(self.DIR_PATH, 'sqlite'))

    def tearDown(self) -> None:
        clear_dt_project(self.DIR_PATH)

    def test_ms_run(self):
        with mock.patch(NAMESPACE + ".ClusterHCCLViewModel.get_hccl_op_data", return_value=[[1, 2]]), \
                mock.patch(NAMESPACE + ".HwtsIterViewModel.get_ai_core_op_data", return_value=[[3, 4]]), \
                mock.patch(NAMESPACE + ".TsTrackViewModel.get_ai_cpu_op_data", return_value=[[3, 4]]), \
                mock.patch(NAMESPACE + ".TsTrackViewModel.get_iter_time_data", return_value=[[5, 6]]), \
                mock.patch(NAMESPACE + ".SectionCalculator.merge_continuous_intervals"), \
                mock.patch(NAMESPACE + ".SectionCalculator.compute_overlap_time", side_effect=self.SIDE_EFFECT):
            check = ClusterParallelParser(self.FILE_LIST_2, self.SAMPLE_CONFIG)
            check.ms_run()
        with ParallelViewModel(self.DIR_PATH) as _model:
            sql1 = "select count(0) from HcclOperatorOverlap"
            sql2 = "select count(0) from ComputationTime"
            self.assertEqual(not DBManager.fetch_all_data(_model.cur, sql1), False)
            self.assertEqual(not DBManager.fetch_all_data(_model.cur, sql2), False)

    def test_ms_run_without_hccl_op_data(self):
        with mock.patch(NAMESPACE + ".ClusterHCCLViewModel.get_hccl_op_data", return_value=[]):
            check = ClusterParallelParser(self.FILE_LIST_2, self.SAMPLE_CONFIG)
            check.ms_run()
        with ParallelViewModel(self.DIR_PATH) as _model:
            sql1 = "select count(0) from HcclOperatorOverlap"
            sql2 = "select count(0) from ComputationTime"
            self.assertEqual(not DBManager.fetch_all_data(_model.cur, sql1), True)
            self.assertEqual(not DBManager.fetch_all_data(_model.cur, sql2), True)

    def test_ms_run_without_compute_op_data(self):
        with mock.patch(NAMESPACE + ".ClusterHCCLViewModel.get_hccl_op_data", return_value=[[1, 2]]), \
                mock.patch(NAMESPACE + ".HwtsIterViewModel.get_ai_core_op_data", return_value=[]), \
                mock.patch(NAMESPACE + ".TsTrackViewModel.get_ai_cpu_op_data", return_value=[]):
            check = ClusterParallelParser(self.FILE_LIST_2, self.SAMPLE_CONFIG)
            check.ms_run()
        with ParallelViewModel(self.DIR_PATH) as _model:
            sql1 = "select count(0) from HcclOperatorOverlap"
            sql2 = "select count(0) from ComputationTime"
            self.assertEqual(not DBManager.fetch_all_data(_model.cur, sql1), True)
            self.assertEqual(not DBManager.fetch_all_data(_model.cur, sql2), True)

    def test_ms_run_without_step_trace_data(self):
        with mock.patch(NAMESPACE + ".ClusterHCCLViewModel.get_hccl_op_data", return_value=[[1, 2]]), \
                mock.patch(NAMESPACE + ".HwtsIterViewModel.get_ai_core_op_data", return_value=[[3, 4]]), \
                mock.patch(NAMESPACE + ".TsTrackViewModel.get_ai_cpu_op_data", return_value=[[3, 4]]), \
                mock.patch(NAMESPACE + ".TsTrackViewModel.get_iter_time_data", return_value=[]):
            check = ClusterParallelParser(self.FILE_LIST_2, self.SAMPLE_CONFIG)
            check.ms_run()
        with ParallelViewModel(self.DIR_PATH) as _model:
            sql1 = "select count(0) from HcclOperatorOverlap"
            sql2 = "select count(0) from ComputationTime"
            self.assertEqual(not DBManager.fetch_all_data(_model.cur, sql1), True)
            self.assertEqual(not DBManager.fetch_all_data(_model.cur, sql2), True)

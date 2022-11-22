#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

import os
import unittest
from unittest import mock

from common_func.db_manager import DBManager
from constant.constant import clear_dt_project
from constant.info_json_construct import DeviceInfo, InfoJson, InfoJsonReaderManager
from msmodel.parallel.cluster_parallel_model import ClusterParallelViewModel
from msparser.parallel.cluster_parallel_collector import ClusterParallelCollector
from profiling_bean.db_dto.cluster_rank_dto import ClusterRankDto

NAMESPACE = "msparser.parallel.cluster_parallel_collector"


class TestClusterParallelCollector(unittest.TestCase):
    DIR_PATH = os.path.join(os.path.dirname(__file__), 'DT_ClusterParallelCollector')
    CLUSTRER_INFO = [ClusterRankDto()]
    DATA_PARALLEL_DATA = [[1, 2, 3, 4, 5, 6, 7, 8, 9, 10]]
    MODEL_PARALLEL_DATA = [[1, 2, 3, 4, 5, 6, 7, 8]]
    PIPELINE_PARALLEL_DATA = [[1, 2, 3, 4, 5, 6, 7, 8, 9, 10]]
    PARALLEL_STRATEGY_DATA = [[1, 2, 3, 4, 5, 6, 7]]

    def setUp(self) -> None:
        os.makedirs(os.path.join(self.DIR_PATH, 'PROF1', 'device_0'))
        os.makedirs(os.path.join(self.DIR_PATH, 'sqlite'))

    def tearDown(self) -> None:
        clear_dt_project(self.DIR_PATH)

    def test_parse(self) -> None:
        self.CLUSTRER_INFO[0].rank_id = 0
        self.CLUSTRER_INFO[0].dir_name = "PROF1/device_0"
        with mock.patch(NAMESPACE + ".ClusterInfoViewModel.check_table", return_value=True), \
                mock.patch(NAMESPACE + ".ClusterInfoViewModel.get_all_cluster_rank_info",
                           return_value=self.CLUSTRER_INFO), \
                mock.patch(NAMESPACE + ".ParallelViewModel.get_parallel_table_name",
                           return_value="ClusterDataParallel"), \
                mock.patch(NAMESPACE + ".ClusterParallelCollector.multithreading_get_parallel_data"):
            check = ClusterParallelCollector(self.DIR_PATH)
            check.parse()

    def test_ms_run_data_parallel(self) -> None:
        self.CLUSTRER_INFO[0].rank_id = 1
        self.CLUSTRER_INFO[0].dir_name = "PROF1/device_0"
        InfoJsonReaderManager(info_json=InfoJson(DeviceInfo=[DeviceInfo(hwts_frequency=100).device_info])).process()
        with mock.patch(NAMESPACE + ".ClusterInfoViewModel.check_table", return_value=True), \
                mock.patch(NAMESPACE + ".ClusterInfoViewModel.get_all_cluster_rank_info",
                           return_value=self.CLUSTRER_INFO), \
                mock.patch(NAMESPACE + ".ParallelViewModel.get_parallel_table_name",
                           return_value="ClusterDataParallel"), \
                mock.patch(NAMESPACE + ".ParallelViewModel.get_parallel_index_data",
                           return_value=self.DATA_PARALLEL_DATA), \
                mock.patch(NAMESPACE + ".ParallelViewModel.get_parallel_strategy_data",
                           return_value=self.PARALLEL_STRATEGY_DATA):
            check = ClusterParallelCollector(self.DIR_PATH)
            check.ms_run()
            with ClusterParallelViewModel(self.DIR_PATH) as _model:
                sql = "select * from ClusterDataParallel"
                self.assertEqual(not DBManager.fetch_all_data(_model.cur, sql), False)

    def test_ms_run_model_parallel(self) -> None:
        self.CLUSTRER_INFO[0].rank_id = 1
        self.CLUSTRER_INFO[0].device_id = 1
        self.CLUSTRER_INFO[0].dir_name = "PROF1/device_0"
        InfoJsonReaderManager(info_json=InfoJson(DeviceInfo=[DeviceInfo(hwts_frequency=100).device_info])).process()
        with mock.patch(NAMESPACE + ".ClusterInfoViewModel.check_table", return_value=True), \
                mock.patch(NAMESPACE + ".ClusterInfoViewModel.get_all_cluster_rank_info",
                           return_value=self.CLUSTRER_INFO), \
                mock.patch(NAMESPACE + ".ParallelViewModel.get_parallel_table_name",
                           return_value="ClusterModelParallel"), \
                mock.patch(NAMESPACE + ".ParallelViewModel.get_parallel_index_data",
                           return_value=self.MODEL_PARALLEL_DATA), \
                mock.patch(NAMESPACE + ".ParallelViewModel.get_parallel_strategy_data",
                           return_value=self.PARALLEL_STRATEGY_DATA):
            check = ClusterParallelCollector(self.DIR_PATH)
            check.ms_run()
            with ClusterParallelViewModel(self.DIR_PATH) as _model:
                sql = "select * from ClusterModelParallel"
                self.assertEqual(not DBManager.fetch_all_data(_model.cur, sql), False)

    def test_ms_run_pipeline_parallel(self) -> None:
        self.CLUSTRER_INFO[0].rank_id = 1
        self.CLUSTRER_INFO[0].dir_name = "PROF1/device_0"
        InfoJsonReaderManager(info_json=InfoJson(DeviceInfo=[DeviceInfo(hwts_frequency=100).device_info])).process()
        with mock.patch(NAMESPACE + ".ClusterInfoViewModel.check_table", return_value=True), \
                mock.patch(NAMESPACE + ".ClusterInfoViewModel.get_all_cluster_rank_info",
                           return_value=self.CLUSTRER_INFO), \
                mock.patch(NAMESPACE + ".ParallelViewModel.get_parallel_table_name",
                           return_value="ClusterPipelineParallel"), \
                mock.patch(NAMESPACE + ".ParallelViewModel.get_parallel_index_data",
                           return_value=self.PIPELINE_PARALLEL_DATA), \
                mock.patch(NAMESPACE + ".ParallelViewModel.get_parallel_strategy_data",
                           return_value=self.PARALLEL_STRATEGY_DATA):
            check = ClusterParallelCollector(self.DIR_PATH)
            check.ms_run()
            with ClusterParallelViewModel(self.DIR_PATH) as _model:
                sql = "select * from ClusterPipelineParallel"
                self.assertEqual(not DBManager.fetch_all_data(_model.cur, sql), False)

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
from msmodel.parallel.parallel_model import ParallelViewModel
from msparser.parallel.parallel_strategy_parser import ParallelStrategyParser
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = "msparser.parallel.parallel_strategy_parser"


class TestParallelStrategyParser(unittest.TestCase):
    FILE_LIST_1 = {1: ["test"]}
    FILE_LIST_2 = {DataTag.PARALLEL_STRATEGY: ["test"]}
    DIR_PATH = os.path.join(os.path.dirname(__file__), 'DT_ParallelStrategyParser')
    SAMPLE_CONFIG = {"result_dir": DIR_PATH}

    def setUp(self) -> None:
        os.makedirs(os.path.join(self.DIR_PATH, 'PROF1', 'device_0'))
        os.makedirs(os.path.join(self.DIR_PATH, 'sqlite'))

    def tearDown(self) -> None:
        clear_dt_project(self.DIR_PATH)

    def test_parse_should_raise_prof_exception_when_without_file(self):
        check = ParallelStrategyParser(self.FILE_LIST_1, self.SAMPLE_CONFIG)
        check.parse([])
        with ParallelViewModel(self.DIR_PATH) as _model:
            self.assertEqual(_model.get_parallel_table_name(), Constant.NA)

    def test_ms_run_should_be_data_parallel(self):
        with mock.patch(NAMESPACE + ".PathManager.get_data_file_path"), \
                mock.patch(NAMESPACE + ".check_file_readable"), \
                mock.patch(NAMESPACE + ".FileOpen"), \
                mock.patch(NAMESPACE + ".FileOpen.file_reader.read"), \
                mock.patch(NAMESPACE + ".json.loads",
                           return_value={"config": {"parallelType": "data_parallel", "stage_num": 1}}):
            check = ParallelStrategyParser(self.FILE_LIST_2, self.SAMPLE_CONFIG)
            check.ms_run()
            with ParallelViewModel(self.DIR_PATH) as _model:
                self.assertEqual(_model.get_parallel_table_name(), "ClusterDataParallel")

    def test_ms_run_should_be_model_parallel(self):
        with mock.patch(NAMESPACE + ".PathManager.get_data_file_path"), \
                mock.patch(NAMESPACE + ".check_file_readable"), \
                mock.patch(NAMESPACE + ".FileOpen"), \
                mock.patch(NAMESPACE + ".FileOpen.file_reader.read"), \
                mock.patch(NAMESPACE + ".json.loads",
                           return_value={"config": {"parallelType": "auto_parallel", "stage_num": 1}}):
            check = ParallelStrategyParser(self.FILE_LIST_2, self.SAMPLE_CONFIG)
            check.ms_run()
        with ParallelViewModel(self.DIR_PATH) as _model:
            self.assertEqual(_model.get_parallel_table_name(), "ClusterModelParallel")

    def test_ms_run_should_be_pipeline_parallel(self):
        with mock.patch(NAMESPACE + ".PathManager.get_data_file_path"), \
                mock.patch(NAMESPACE + ".check_file_readable"), \
                mock.patch(NAMESPACE + ".FileOpen"), \
                mock.patch(NAMESPACE + ".FileOpen.file_reader.read"), \
                mock.patch(NAMESPACE + ".json.loads", return_value={"config": {"stage_num": 2}}):
            check = ParallelStrategyParser(self.FILE_LIST_2, self.SAMPLE_CONFIG)
            check.ms_run()
        with ParallelViewModel(self.DIR_PATH) as _model:
            self.assertEqual(_model.get_parallel_table_name(), "ClusterPipelineParallel")

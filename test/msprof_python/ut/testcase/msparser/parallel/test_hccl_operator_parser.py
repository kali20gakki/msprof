#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

import os
import unittest
from unittest import mock

import pytest

from common_func.msprof_exception import ProfException
from constant.constant import clear_dt_project
from msmodel.parallel.cluster_hccl_model import ClusterHCCLViewModel
from msparser.parallel.hccl_operator_parser import HCCLOperatiorParser
from profiling_bean.db_dto.step_trace_ge_dto import StepTraceGeDto
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = "msparser.parallel.hccl_operator_parser"


class TestHCCLOperatiorParser(unittest.TestCase):
    FILE_LIST_1 = {1: ["test"]}
    FILE_LIST_2 = {DataTag.PARALLEL_STRATEGY: ["test"]}
    DIR_PATH = os.path.join(os.path.dirname(__file__), 'DT_HCCLOperatiorParser')
    SAMPLE_CONFIG = {"result_dir": DIR_PATH}

    def setUp(self) -> None:
        os.makedirs(os.path.join(self.DIR_PATH, 'PROF1', 'device_0'))
        os.makedirs(os.path.join(self.DIR_PATH, 'sqlite'))

    def tearDown(self) -> None:
        clear_dt_project(self.DIR_PATH)

    def test_ms_run(self):
        hccl_data1 = StepTraceGeDto()
        hccl_data1.tag_id = 10000
        hccl_data1.timestamp = 33
        hccl_data2 = StepTraceGeDto()
        hccl_data2.tag_id = 10001
        hccl_data2.timestamp = 44
        with mock.patch(NAMESPACE + ".TsTrackViewModel.get_hccl_operator_exe_data",
                        return_value=[hccl_data1, hccl_data2]), \
                mock.patch(NAMESPACE + ".GeHashViewModel.get_ge_hash_data", return_value={}):
            check = HCCLOperatiorParser(self.FILE_LIST_2, self.SAMPLE_CONFIG)
            check.ms_run()
        with ClusterHCCLViewModel(self.DIR_PATH) as _model:
            data = _model.get_hccl_op_data()
        self.assertEqual(not data, False)

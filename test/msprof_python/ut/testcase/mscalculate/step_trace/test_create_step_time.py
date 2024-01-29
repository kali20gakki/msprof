#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.

import unittest
from unittest import mock

from mscalculate.step_trace.create_step_table import CreateStepTime
from profiling_bean.db_dto.step_trace_dto import StepTraceOriginDto

NAMESPACE = 'mscalculate.step_trace.create_step_table'


class TestCreateStepTime(unittest.TestCase):
    def test_extract_data_should_return_2_data_when_2_step_is_running(self):
        step_data = [
            StepTraceOriginDto(1, 4294967295, 0, 60000, 0, 1000),
            StepTraceOriginDto(1, 4294967295, 0, 60001, 4, 1300),
            StepTraceOriginDto(2, 4294967295, 0, 60000, 6, 1500),
            StepTraceOriginDto(2, 4294967295, 0, 60001, 9, 1900),
        ]
        CreateStepTime.sample_config = {"result_dir": "./"}
        with mock.patch(NAMESPACE + ".TsTrackModel.get_step_trace_with_tag", return_value=step_data):
            CreateStepTime.extract_data()
            CreateStepTime.connect_db()
            res = [
                [1, 4294967295, 1000, 1300, 1],
                [2, 4294967295, 1500, 1900, 2],
            ]
            self.assertEqual(res, CreateStepTime.data)
        CreateStepTime.sample_config = {}
        CreateStepTime.data = []

    def test_extract_data_should_return_1_data_when_there_is_only_start_tag(self):
        step_data = [
            StepTraceOriginDto(1, 4294967295, 0, 60000, 0, 1000),
            StepTraceOriginDto(1, 4294967295, 0, 60001, 4, 1300),
            StepTraceOriginDto(2, 4294967295, 0, 60000, 6, 1500),
        ]
        CreateStepTime.sample_config = {"result_dir": "./"}
        with mock.patch(NAMESPACE + ".TsTrackModel.get_step_trace_with_tag", return_value=step_data):
            CreateStepTime.extract_data()
            res = [
                [1, 4294967295, 1000, 1300, 1],
            ]
            self.assertEqual(res, CreateStepTime.data)
        CreateStepTime.sample_config = {}
        CreateStepTime.data = []

#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.

import unittest
from unittest import mock

from msmodel.step_trace.ts_track_model import TsTrackViewModel

NAMESPACE = "msmodel.step_trace.ts_track_model"


class TestTsTrackViewModel(unittest.TestCase):
    def test_get_hccl_operator_exe_data_without_ge(self):
        with mock.patch(NAMESPACE + ".TsTrackViewModel.attach_to_db", return_value=False):
            with TsTrackViewModel("test") as _model:
                self.assertEqual(_model.get_hccl_operator_exe_data(), [])

    def test_get_hccl_operator_exe_data(self):
        with mock.patch(NAMESPACE + ".TsTrackViewModel.attach_to_db", return_value=True), \
                mock.patch(NAMESPACE + ".DBManager.fetch_all_data", return_value=[[1]]):
            with TsTrackViewModel("test") as _model:
                self.assertEqual(_model.get_hccl_operator_exe_data(), [[1]])

    def test_get_ai_cpu_op_data(self):
        with mock.patch(NAMESPACE + ".DBManager.fetch_all_data", return_value=[[2]]):
            with TsTrackViewModel("test") as _model:
                self.assertEqual(_model.get_ai_cpu_op_data(), [[2]])

    def test_get_iter_time_data(self):
        with mock.patch(NAMESPACE + ".DBManager.fetch_all_data", return_value=[[3]]):
            with TsTrackViewModel("test") as _model:
                self.assertEqual(_model.get_iter_time_data(), [[3]])

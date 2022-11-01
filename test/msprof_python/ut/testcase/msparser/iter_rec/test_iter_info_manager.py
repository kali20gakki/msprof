#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2022. All rights reserved.
"""
import unittest
from unittest import mock

from msmodel.ge.ge_info_calculate_model import GeInfoModel
from msparser.iter_rec.iter_info_updater.iter_info_manager import IterInfoManager

from mock_tools import ClassMock


class TestIterInfoUpdater(unittest.TestCase):
    def test_initial_iter_to_info_1(self: any) -> None:
        instance = mock.Mock()
        instance.project_path = ""
        with mock.patch("common_func.msvp_common.path_check", return_value=False):
            IterInfoManager.initial_iter_to_info(instance)

    def test_initial_iter_to_info_2(self: any) -> None:
        with mock.patch("common_func.msvp_common.path_check" , return_value=True), \
            mock.patch("common_func.path_manager.PathManager.get_db_path"):
            ge_mock = mock.Mock
            ge_mock.__enter__ = GeInfoModel.__enter__
            ge_mock.__exit__ = GeInfoModel.__exit__
            with ClassMock(GeInfoModel, ge_mock()):
                IterInfoManager.initial_iter_to_info(mock.Mock())

    def test_regist_parallel_set(self: any) -> None:
        instance = mock.Mock()
        instance.iter_to_iter_info = {}
        step_trace_datum = mock.Mock()

        step_trace_datum.model_id = 1
        step_trace_datum.index_id = 1
        step_trace_datum.iter_id = 1
        step_trace_datum.step_start = 1
        step_trace_datum.step_end = 1

        step_trace_data = [step_trace_datum, step_trace_datum]

        IterInfoManager.regist_parallel_set(instance, step_trace_data)

    def test_regist_aicore_set(self: any) -> None:
        instance = mock.Mock()
        iter_info_bean = mock.Mock()
        iter_info_bean.model_id = 1
        iter_info_bean.index_id = 2
        instance.iter_to_iter_info = {1: iter_info_bean}

        IterInfoManager.regist_aicore_set(instance, {}, {})

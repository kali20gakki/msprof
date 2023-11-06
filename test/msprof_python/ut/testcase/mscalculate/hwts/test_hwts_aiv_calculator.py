#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import unittest
from unittest import mock

from common_func.profiling_scene import ProfilingScene
from constant.constant import CONFIG
from mscalculate.hwts.hwts_aiv_calculator import HwtsAivCalculator
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'mscalculate.hwts.hwts_aiv_calculator'


class TestHwtsAivCalculator(unittest.TestCase):
    file_list = {DataTag.AIV: ['aiVectorCore.data.0.slice_0']}

    def test_ms_run_when_table_not_exist_then_parse_and_save(self):
        with mock.patch('common_func.db_manager.DBManager.check_tables_in_db', return_value=False):
            ProfilingScene()._scene = "single_op"
            check = HwtsAivCalculator(self.file_list, CONFIG)
            check._parse_all_file = mock.Mock()
            check.save = mock.Mock()
            check.ms_run()
            check._parse_all_file.assert_called_once()
            check.save.assert_called_once()

    def test_ms_run_when_table_exist_then_do_not_execute(self):
        with mock.patch('common_func.db_manager.DBManager.check_tables_in_db', return_value=True), \
                mock.patch('logging.info'):
            ProfilingScene()._scene = "single_op"
            check = HwtsAivCalculator(self.file_list, CONFIG)
            check._parse_all_file = mock.Mock()
            check.save = mock.Mock()
            check.ms_run()
            check._parse_all_file.assert_not_called()
            check.save.assert_not_called()

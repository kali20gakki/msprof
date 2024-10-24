#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
import unittest
from unittest import mock

from constant.constant import CONFIG
from common_func.profiling_scene import ProfilingScene
from common_func.profiling_scene import ExportMode
from common_func.info_conf_reader import InfoConfReader
from mscalculate.hccl.kfc_calculator import KfcCalculator
from mscalculate.ascend_task.ascend_task import TopDownTask
from msmodel.add_info.mc2_comm_info_model import Mc2CommInfoViewModel

NAMESPACE = 'mscalculate.hccl.kfc_calculator'


class TestKfcCalculator(unittest.TestCase):
    def test_ms_run_should_return_when_no_kfc_info_db(self: any) -> None:
        check = KfcCalculator([], CONFIG)
        check.ms_run()

    def test_ms_run_should_drop_table_when_has_kfc_info_db_and_calculate_again(self: any) -> None:
        with mock.patch("os.path.exists", return_value=True), \
                mock.patch(NAMESPACE + ".KfcCalculator._judge_calculate_again", return_value=True), \
                mock.patch(NAMESPACE + ".KfcCalculator.calculate"):
            check = KfcCalculator([], CONFIG)
            check.ms_run()

    def test_judge_calculate_again_should_return_true_when_not_all_export(self: any) -> None:
        ProfilingScene().set_mode(ExportMode.GRAPH_EXPORT)
        check = KfcCalculator([], CONFIG)
        check._judge_calculate_again()
        ProfilingScene().set_mode(ExportMode.ALL_EXPORT)

    def test_judge_calculate_again_should_return_true_when_all_export_and_not_have_kfc_op_table(self: any) -> None:
        with mock.patch(NAMESPACE + ".DBManager.check_tables_in_db", return_value=False):
            check = KfcCalculator([], CONFIG)
            self.assertTrue(check._judge_calculate_again())

    def test_judge_calculate_again_should_return_false_when_all_export_and_have_kfc_op_table(self: any) -> None:
        with mock.patch(NAMESPACE + ".DBManager.check_tables_in_db", return_value=True):
            check = KfcCalculator([], CONFIG)
            self.assertFalse(check._judge_calculate_again())

    def test_calculate_should_return_2_kfc_op_and_2_kfc_task_when_2_kfc_stream(self: any) -> None:
        group_name = "12345678"
        InfoConfReader()._info_json = {
            "devices": 0
        }
        task_data = [
            TopDownTask(0, 0, 19, 0, 4294967295, 0, 38140478700000, 1500, "KERNEL_AICPU", "AI_CPU", 0),
            TopDownTask(0, 0, 19, 1, 4294967295, 0, 38140478800000, 2000, "KERNEL_AICPU", "AI_CPU", 0),
            TopDownTask(0, 0, 52, 0, 4294967295, 0, 38140478600000, 1000, "UNKNOWN", "C_CORE_SQE", 0),
            TopDownTask(0, 0, 52, 1, 4294967295, 0, 38140478700010, 1000, "KERNEL_AICORE", "C_CORE_SQE", 0),
            TopDownTask(0, 0, 54, 0, 4294967295, 0, 38140478800010, 500, "KERNEL_AICORE", "WRITE_VALUE_SQE", 0),
        ]
        comm_info = [
            Mc2CommInfoViewModel.MC2_COMM_INFO_TYPE(19, "52,53,54,55,56,57,58,59", group_name),
        ]
        ge_data = [
            (4294967295, 'MatmulAllReduceAicpu', 19, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
             4294967295, 0, 0),
            (4294967295, 'MatmulAllReduceAicpu', 19, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
             4294967295, 0, 0),
            (4294967295, 'Matmul', 20, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
             4294967295, 0, 0),
        ]
        kfc_info_data = [
            (1, "hccl_info", 0, group_name, 0, 0, 8, 0, 0, 4294967295, 2, 0, 0, 0.1, 0, 0, 1, 0, 266, 0, 0, 0, 52, 0,
             0, 0, 0, 0),
            (1, "hccl_info", 0, group_name, 0, 0, 8, 0, 0, 4294967295, 2, 0, 0, 0.1, 0, 0, 1, 0, 266, 0, 0, 0, 52, 1,
             0, 0, 0, 0),
            (1, "Notify_Wait", 0, group_name, 0, 0, 8, 0, 0, 4294967295, 2, 0, 0, 0.1, 0, 0, 1, 0, 266, 0, 0, 0, 54, 0,
             0, 0, 0, 0),
        ]
        with mock.patch(NAMESPACE + ".AscendTaskModel.get_ascend_task_data_without_unknown", return_value=task_data), \
                mock.patch(NAMESPACE + ".Mc2CommInfoViewModel.get_kfc_stream", return_value=comm_info), \
                mock.patch(NAMESPACE + ".DBManager.fetch_all_data", return_value=ge_data), \
                mock.patch(NAMESPACE + ".KfcInfoViewModel.get_sql_data", return_value=kfc_info_data), \
                mock.patch(NAMESPACE + ".DBManager.judge_table_exist", return_value=True), \
                mock.patch("os.path.exists", return_value=True), \
                mock.patch("msmodel.step_trace.ts_track_model.TsTrackModel.get_task_flip_data", return_value=[]), \
                mock.patch("common_func.db_manager.DBManager.check_connect_db", return_value=(True, True)):
            check = KfcCalculator([], CONFIG)
            check.calculate()
            check.save()
        self.assertEqual(2, len(check._kfc_op_data))
        self.assertEqual(2, len(check._kfc_task_data))
        InfoConfReader()._info_json = {}

    def test_make_default_kfc_info_should_return_len_28_named_tuple(self: any) -> None:
        default_kfc_info = KfcCalculator.make_default_kfc_info()
        self.assertEqual(28, len(default_kfc_info))

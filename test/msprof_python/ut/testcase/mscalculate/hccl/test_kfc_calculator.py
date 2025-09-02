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
from msparser.step_trace.ts_binary_data_reader.task_flip_bean import TaskFlip
from msmodel.add_info.mc2_comm_info_model import Mc2CommInfoViewModel
from msmodel.task_time.ascend_task_model import AscendTaskViewModel

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

    def test_calculate_should_return_2_kfc_op_and_11_kfc_task_when_2_kfc_stream(self: any) -> None:
        group_name = "group"
        InfoConfReader()._info_json = {
            "devices": 0
        }
        kfc_op_data = [
            # hccl aicpu kernel, 异步
            AscendTaskViewModel.ASCEND_TASK_TYPE(0, 0, 19, 0, 4294967295, 0, 38140478700000,
                                                 100000, "KERNEL_AICPU", "AI_CPU", 0, "allgatherAicpuKernel", -1),
            # hccl aicpu kernel, 异步 stream 29 异常数据,和host数据无法对齐
            AscendTaskViewModel.ASCEND_TASK_TYPE(0, 0, 29, 0, 4294967295, 0, 38140478700000,
                                                 100000, "KERNEL_AICPU", "AI_CPU", 0, "allgatherAicpuKernel", -1),

            # mc2 aicpu kernel, 同步
            AscendTaskViewModel.ASCEND_TASK_TYPE(0, 0, 19, 2, 4294967295, 0, 38140478900000,
                                                 200000, "KERNEL_AICPU", "AI_CPU", 0, "MatmulAllReduceAicpu", -1),
        ]
        kfc_task_data = [
            AscendTaskViewModel.ASCEND_TASK_TYPE(0, 0, 52, 0, 4294967295, 0, 38140478800000,  # stream 52,hccl aicpu主流
                                                 1000, "UNKNOWN", "NOTIFY_WAIT_SQE", 0, "", -1),
            AscendTaskViewModel.ASCEND_TASK_TYPE(0, 0, 52, 1, 4294967295, 0, 38140478802000,
                                                 1000, "UNKNOWN", "SDMA_SQE", 0, "", -1),
            # 发生重执行, task将不被执行 ===============================================================================
            # AscendTaskViewModel.ASCEND_TASK_TYPE(0, 0, 52, 2, 4294967295, 0, 38140478803000,
            #                                      1000, "UNKNOWN", "NOTIFY_RECORD_SQE", 0, ""),
            # AscendTaskViewModel.ASCEND_TASK_TYPE(0, 0, 52, 1, 4294967295, 0, 38140478805000,  # task id 翻转
            #                                      1000, "UNKNOWN", "NOTIFY_WAIT_SQE", 0, ""),
            # AscendTaskViewModel.ASCEND_TASK_TYPE(0, 0, 52, 2, 4294967295, 0, 38140478810000,
            #                                      1000, "UNKNOWN", "SDMA_SQE", 0, ""),
            # AscendTaskViewModel.ASCEND_TASK_TYPE(0, 0, 52, 1, 4294967295, 0, 38140478830000,  # task id 翻转
            #                                      1000, "UNKNOWN", "SDMA_SQE", 0, ""),
            # AscendTaskViewModel.ASCEND_TASK_TYPE(0, 0, 52, 2, 4294967295, 0, 38140478840000,
            #                                      1000, "UNKNOWN", "NOTIFY_RECORD_SQE", 0, ""),
            # =======================================================================================
            # 开始重执行，上报重执行flip
            AscendTaskViewModel.ASCEND_TASK_TYPE(0, 0, 52, 5, 4294967295, 0, 38140478860000,
                                                 1000, "UNKNOWN", "SDMA_SQE", 0, "", -1),
            AscendTaskViewModel.ASCEND_TASK_TYPE(0, 0, 52, 1, 4294967295, 0, 38140478870000,  # task id 翻转
                                                 1000, "UNKNOWN", "SDMA_SQE", 0, "", -1),
            AscendTaskViewModel.ASCEND_TASK_TYPE(0, 0, 52, 2, 4294967295, 0, 38140478880000,
                                                 1000, "UNKNOWN", "NOTIFY_RECORD_SQE", 0, "", -1),

            AscendTaskViewModel.ASCEND_TASK_TYPE(0, 0, 53, 0, 4294967295, 0, 38140478830000,  # stream 53,hccl aicpu从流
                                                 500, "UNKNOWN", "NOTIFY_WAIT_SQE", 0, "", -1),
            AscendTaskViewModel.ASCEND_TASK_TYPE(0, 0, 53, 1, 4294967295, 0, 38140478840010,
                                                 500, "UNKNOWN", "SDMA_SQE", 0, "", -1),
            AscendTaskViewModel.ASCEND_TASK_TYPE(0, 0, 53, 2, 4294967295, 0, 38140478870010,
                                                 500, "UNKNOWN", "NOTIFY_RECORD_SQE", 0, "", -1),

            AscendTaskViewModel.ASCEND_TASK_TYPE(0, 0, 54, 0, 4294967295, 0, 38140478900010,  # stream 54, mc2主流
                                                 500, "UNKNOWN", "C_CORE_SQE", 0, "", -1),
            AscendTaskViewModel.ASCEND_TASK_TYPE(0, 0, 54, 1, 4294967295, 0, 38140478920010,
                                                 500, "UNKNOWN", "SDMA_SQE", 0, "", -1),
            AscendTaskViewModel.ASCEND_TASK_TYPE(0, 0, 54, 2, 4294967295, 0, 38140478940010,
                                                 500, "UNKNOWN", "NOTIFY_WAIT", 0, "", -1),
        ]
        master_stream_hccl_task = [
            (38140478700010, 19, 0, 52, 0, 0, 0, 0),  # batch_id = 0
            (38140478707100, 19, 0, 52, 2, 0, 0, 1),  # batch_id = 2
            (38140478715000, 19, 0, 52, 2, 0, 0, 1),  # batch_id = 3
            (38140478700010, 29, 0, 80, 0, 0, 0, 0),  # batch_id = 0 异常数据,和host数据无法对齐
        ]
        aicpu_task_flip = [
            (52, 38140478704000, 0, 1),  # task id翻转
            (52, 38140478707000, 0, 2),  # task id翻转
            (52, 38140478710000, 4, 2),  # 重执行上报flip
            (52, 38140478712000, 0, 3),  # task id翻转
            (80, 38140478704000, 0, 1),  # task id翻转 异常数据,和host数据无法对齐
        ]
        ts_task_flip = [
            TaskFlip(52, 38140478850000, 4, 2),  # 重执行上报flip
            TaskFlip(52, 38140478865000, 0, 3),  # task id翻转
        ]
        comm_info = [
            Mc2CommInfoViewModel.MC2_COMM_INFO_TYPE(group_name, 8, 0, 0, 19, "52,53,54,55,56,57,58,59"),
            Mc2CommInfoViewModel.MC2_COMM_INFO_TYPE(group_name, 2, 0, 0, 30, "80,81"), # 异常数据,和host数据无法对齐
        ]
        ge_data = [
            (4294967295, 'allgather', 19, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
             4294967295, 0, 0),
            (4294967295, 'MatmulAllReduceAicpu', 19, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
             4294967295, 0, 0),
            (4294967295, 'Matmul', 20, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
             4294967295, 0, 0),
        ]
        hccl_info_data = [
            (38140478701100, "NOTIFY_WAIT_SQE", 0, group_name, 0, 0, 8, 0, 0, 4294967295, 2, 0, 0, 0.1, 0, 0, 1, 0,
             266, 0, 0, 0, 52, 0, 0, 0, 0, 0, 'NA', -1),
            (38140478702000, "SDMA_SQE", 0, group_name, 0, 0, 8, 0, 0, 4294967295, 2, 0, 0, 0.1, 0, 0, 1, 0,
             266, 0, 0, 0, 52, 1, 0, 0, 0, 0, 'NA', -1),
            (38140478703300, "NOTIFY_RECORD_SQE", 0, group_name, 0, 0, 8, 0, 0, 4294967295, 2, 0, 0, 0.1, 0, 0, 1, 0,
             266, 0, 0, 0, 52, 2, 0, 0, 0, 0, 'NA', -1),
            # taskid翻转
            (38140478705000, "NOTIFY_WAIT_SQE", 0, group_name, 0, 0, 8, 0, 0, 4294967295, 2, 0, 0, 0.1, 0, 0, 1, 0,
             266, 0, 0, 0, 52, 1, 0, 0, 0, 0, 'NA', -1),
            (38140478706000, "SDMA_SQE", 0, group_name, 0, 0, 8, 0, 0, 4294967295, 2, 0, 0, 0.1, 0, 0, 1, 0,
             266, 0, 0, 0, 52, 2, 0, 0, 0, 0, 'NA', -1),
            # taskid翻转
            (38140478707200, "SDMA_SQE", 0, group_name, 0, 0, 8, 0, 0, 4294967295, 2, 0, 0, 0.1, 0, 0, 1, 0,
             266, 0, 0, 0, 52, 1, 0, 0, 0, 0, 'NA', -1),
            (38140478708000, "NOTIFY_RECORD_SQE", 0, group_name, 0, 0, 8, 0, 0, 4294967295, 2, 0, 0, 0.1, 0, 0, 1, 0,
             266, 0, 0, 0, 52, 2, 0, 0, 0, 0, 'NA', -1),
            (38140478711000, "SDMA_SQE", 0, group_name, 0, 0, 8, 0, 0, 4294967295, 2, 0, 0, 0.1, 0, 0, 1, 0,
             266, 0, 0, 0, 52, 5, 0, 0, 0, 0, 'NA', -1),
            (38140478712100, "SDMA_SQE", 0, group_name, 0, 0, 8, 0, 0, 4294967295, 2, 0, 0, 0.1, 0, 0, 1, 0,
             266, 0, 0, 0, 52, 1, 0, 0, 0, 0, 'NA', -1),
            (38140478712200, "NOTIFY_RECORD_SQE", 0, group_name, 0, 0, 8, 0, 0, 4294967295, 2, 0, 0, 0.1, 0, 0, 1, 0,
             266, 0, 0, 0, 52, 2, 0, 0, 0, 0, 'NA', -1),

            (38140478702000, "NOTIFY_RECORD_SQE", 0, group_name, 0, 0, 8, 0, 0, 4294967295, 2, 0, 0, 0.1, 0, 0, 1, 0,
             266, 0, 0, 0, 53, 0, 0, 0, 0, 0, 'NA', -1),
            (38140478703000, "SDMA_SQE", 0, group_name, 0, 0, 8, 0, 0, 4294967295, 2, 0, 0, 0.1, 0, 0, 1, 0,
             266, 0, 0, 0, 53, 1, 0, 0, 0, 0, 'NA', -1),
            (38140478704000, "NOTIFY_RECORD_SQE", 0, group_name, 0, 0, 8, 0, 0, 4294967295, 2, 0, 0, 0.1, 0, 0, 1, 0,
             266, 0, 0, 0, 53, 2, 0, 0, 0, 0, 'NA', -1),

            (38140478901000, "C_CORE_SQE", 0, group_name, 0, 0, 8, 0, 0, 4294967295, 2, 0, 0, 0.1, 0, 0, 1, 0,
             266, 0, 0, 0, 54, 0, 0, 0, 0, 0, 'NA', -1),
            (38140478922000, "SDMA_SQE", 0, group_name, 0, 0, 8, 0, 0, 4294967295, 2, 0, 0, 0.1, 0, 0, 1, 0,
             266, 0, 0, 0, 54, 1, 0, 0, 0, 0, 'NA', -1),
            (38140478941000, "NOTIFY_WAIT_SQE", 0, group_name, 0, 0, 8, 0, 0, 4294967295, 2, 0, 0, 0.1, 0, 0, 1, 0,
             266, 0, 0, 0, 54, 2, 0, 0, 0, 0, 'NA', -1),
        ]
        hccl_op_info_data = [
            (38140478600000, 0, 0, 0, 90, 1, group_name, 19, 0, 8, 0),
            (38140478702000, 0, 1, 0, 90, 1, group_name, 19, 1, 8, 0),
            (38140478801000, 0, 0, 0, 90, 1, group_name, 19, 2, 8, 1),
            (38140478803000, 0, 0, 0, 90, 1, group_name, 19, 3, 8, 0),
        ]
        with mock.patch(NAMESPACE + ".AscendTaskViewModel.get_ascend_task_data_with_op_name_pattern_and_stream_id",
                        return_value=kfc_op_data), \
                mock.patch(NAMESPACE + ".AscendTaskViewModel.get_ascend_task_data_with_stream_id",
                           return_value=kfc_task_data), \
                mock.patch(NAMESPACE + ".Mc2CommInfoViewModel.get_kfc_stream", return_value=comm_info), \
                mock.patch(NAMESPACE + ".DBManager.fetch_all_data", side_effect=(ge_data, [])), \
                mock.patch(NAMESPACE + ".KfcInfoViewModel.get_sql_data",
                           side_effect=(aicpu_task_flip, hccl_info_data, master_stream_hccl_task, hccl_op_info_data)), \
                mock.patch(NAMESPACE + ".DBManager.judge_table_exist", return_value=True), \
                mock.patch("os.path.exists", return_value=True), \
                mock.patch("msmodel.step_trace.ts_track_model.TsTrackModel.get_task_flip_data",
                           return_value=ts_task_flip), \
                mock.patch("common_func.db_manager.DBManager.check_connect_db", return_value=(True, True)):
            check = KfcCalculator([], CONFIG)
            check.calculate()
            check.save()
        self.assertEqual(2, len(check._kfc_op_data.get(group_name, [])))
        self.assertEqual(11, len(check._kfc_task_data.get(group_name, [])))
        InfoConfReader()._info_json = {}

    def test_make_default_kfc_info_should_return_len_28_named_tuple(self: any) -> None:
        default_kfc_info = KfcCalculator.make_default_kfc_info()
        self.assertEqual(30, len(default_kfc_info))

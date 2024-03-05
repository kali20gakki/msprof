#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023. All rights reserved.
import os
import unittest
from unittest import mock

from common_func.constant import Constant
from common_func.msprof_object import CustomizedNamedtupleFactory
from common_func.profiling_scene import ProfilingScene
from common_func.profiling_scene import ExportMode
from constant.constant import CONFIG
from constant.constant import clear_dt_project
from mscalculate.hccl.hccl_calculator import HcclCalculator
from mscalculate.hccl.hccl_task import HcclOps
from mscalculate.hccl.hccl_task import HcclTask

NAMESPACE = 'mscalculate.hccl.hccl_calculator'

HcclTask = CustomizedNamedtupleFactory.generate_named_tuple_from_dto(HcclTask, [])


class TestHcclCalculator(unittest.TestCase):
    DIR_PATH = os.path.join(os.path.dirname(__file__), 'DT_HcclCalculator')

    @staticmethod
    def construct_hccl_dto(op_name, is_master, timestamp=123456, duration=0, op_type="HCCL_OP_TYPE"):
        hccl_data = HcclTask()
        hccl_data.op_name, hccl_data.iteration, hccl_data.duration, hccl_data.first_timestamp, hccl_data.is_dynamic, \
            hccl_data.model_id, hccl_data.index_id, hccl_data.task_type, hccl_data.op_type, hccl_data.is_master = \
            (op_name, 1, duration, timestamp, 0, 1, 1, "HCCL", op_type, is_master)
        return hccl_data

    def setUp(self) -> None:
        os.makedirs(os.path.join(self.DIR_PATH, 'PROF1', 'device_0'))

    def tearDown(self) -> None:
        clear_dt_project(self.DIR_PATH)

    def test_calculate_should_return_none_when_table_not_in_db(self):
        with mock.patch(NAMESPACE + ".DBManager.check_tables_in_db", return_value=False):
            check = HcclCalculator([], CONFIG)
            ret = check.calculate()
            self.assertIsNone(ret)

    def test_calculate_should_update_both_hccl_data_and_hccl_op_report_data_when_op_type_valid(self):
        with mock.patch(NAMESPACE + ".DBManager.check_tables_in_db", return_value=True), \
                mock.patch(NAMESPACE + '.HcclCalculator._merge_hccl_ops_and_tasks', return_value=[
                    HcclTask(op_name="hccl_op", timestamp=1, duration=1, op_type="all_reduce")]):
            check = HcclCalculator([], CONFIG)
            check.calculate()
            hccl_data = check._hccl_data
            hccl_op_report_data = check._hccl_op_report_data
            self.assertEqual(29, len(hccl_data[0]))
            self.assertEqual([("all_reduce", 1.0, 1.0, 1.0, 1.0, 1.0, 100.0)], hccl_op_report_data)

    def test_calculate_should_update_both_hccl_data_and_hccl_op_report_data_when_op_type_invalid(self):
        with mock.patch(NAMESPACE + ".DBManager.check_tables_in_db", return_value=True), \
                mock.patch(NAMESPACE + '.HcclCalculator._merge_hccl_ops_and_tasks', return_value=[
                    HcclTask(op_name="hccl_op", timestamp=1, duration=1, op_type=Constant.NA)]):
            check = HcclCalculator([], CONFIG)
            check.calculate()
            hccl_data = check._hccl_data
            hccl_op_report_data = check._hccl_op_report_data
            self.assertEqual(29, len(hccl_data[0]))
            self.assertEqual([], hccl_op_report_data)

    def test_generate_hccl_op_info_should_return_three_data_when_the_input_len_is_three(self):
        hccl_data = [
            HcclTask(op_name="hccl_op1", is_master=1),
            HcclTask(op_name="hccl_op1", timestamp=123457, is_master=1),
            HcclTask(op_name="hccl_op2", timestamp=123458, is_master=1)
        ]
        check = HcclCalculator([], CONFIG)
        check._generate_hccl_op_info(hccl_data)
        self.assertEqual(3, len(check._hccl_data))

    def test_cal_total_should_return_5_when_input_is_the_following_task_time(self):
        task_time = {
                     '1': {'count': 2, 'total_time': 4, 'min': 1, 'max': 3, 'avg': 2},
                     '2': {'count': 1, 'total_time': 1, 'min': 1, 'max': 1, 'avg': 1}
        }
        check = HcclCalculator([], CONFIG)
        _cal_total = getattr(check, "_cal_total")
        result = _cal_total(task_time)
        self.assertEqual(result, 5)

    def test_create_report_should_return_list_data_when_input_is_not_empty(self):
        task_data = {
                    '1': {'count': 2, 'total_time': 4, 'min': 1, 'max': 3, 'avg': 2},
                    '2': {'count': 1, 'total_time': 1, 'min': 1, 'max': 1, 'avg': 1}
        }
        check = HcclCalculator([], CONFIG)
        check._create_report(task_data)
        hccl_op_report_data = check._hccl_op_report_data
        self.assertEqual(hccl_op_report_data, [("1", 2, 4.0, 1.0, 2.0, 3.0, 80.0),
                                               ("2", 1, 1.0, 1.0, 1.0, 1.0, 20.0)])

    def test_get_hccl_op_report_data_should_return_dict_data_when_input_is_hccldto_list(self):
        args = {'stream id': 2, 'task id': 0, 'context id': 4294967295, 'is_master': '1'}
        hccl_data = [
            HcclTask(op_name="hccl_op1", is_master=1, timestamp=1, duration=2, op_type="all_reduce"),
            HcclTask(op_name="hccl_op2", is_master=1, timestamp=5, duration=3, op_type="all_reduce"),
            HcclTask(op_name="hccl_op3", is_master=1, timestamp=10, duration=2, op_type="all_gather"),
            HcclTask(op_name="hccl_op4", is_master=1, timestamp=15, duration=3, op_type="all_gather")
        ]
        check = HcclCalculator([], CONFIG)
        hccl_op_report_data = check._get_hccl_op_report_data(hccl_data)
        self.assertEqual(hccl_op_report_data,
                         {"all_reduce": {"count": 2, "total_time": 5, "max": 3, "min": 2, "avg": 2.5},
                          "all_gather": {"count": 2, "total_time": 5, "max": 3, "min": 2, "avg": 2.5}
                          })

    def test_merge_hccl_ops_and_tasks_should_return_five_matched_res_when_input_not_empty(self):
        hccl_ops = [
            HcclOps(timestamp=1, duration=2),
            HcclOps(timestamp=2, duration=2),
            HcclOps(timestamp=3, duration=2),
            HcclOps(timestamp=4, duration=2),
            HcclOps(timestamp=5, duration=2),
            HcclOps(timestamp=6, duration=2),
            HcclOps(timestamp=7, duration=2),
            HcclOps(timestamp=8, duration=2)
        ]

        hccl_tasks = [
            # corner case 1
            HcclTask(host_timestamp=1),
            HcclTask(host_timestamp=3),
            HcclTask(host_timestamp=4),
            HcclTask(host_timestamp=5),
            # corner case 2
            HcclTask(host_timestamp=10),
            # mismatch case 3
            HcclTask(host_timestamp=20)
        ]
        check = HcclCalculator([], CONFIG)
        communication_data = check._merge_hccl_ops_and_tasks(hccl_ops, hccl_tasks)
        self.assertEqual(len(communication_data), 5)


    def test_merge_hccl_ops_and_tasks_should_return_empty_list_when_input_hcclops_empty(self):
        hccl_ops = []
        hccl_tasks = [HcclTask(model_id=4294967295), HcclTask(model_id=4294967296)]
        check = HcclCalculator([], CONFIG)
        communication_data = check._merge_hccl_ops_and_tasks(hccl_ops, hccl_tasks)
        self.assertEqual(communication_data, [])

    def test_merge_hccl_ops_and_tasks_should_return_empty_list_when_input_hccltasks_empty(self):
        hccl_ops = [HcclOps(model_id=4294967295), HcclOps(model_id=4294967296)]
        hccl_tasks = []
        check = HcclCalculator([], CONFIG)
        communication_data = check._merge_hccl_ops_and_tasks(hccl_ops, hccl_tasks)
        self.assertEqual(communication_data, [])

    def test_merge_hccl_ops_and_tasks_should_return_2_data_when_step_export_and_ops_queue_not_empty(self):
        hccl_ops = [HcclOps(model_id=4294967295), HcclOps(model_id=4294967296)]
        hccl_tasks = [HcclTask(model_id=4294967295), HcclTask(model_id=4294967296)]
        ProfilingScene().set_mode(ExportMode.STEP_EXPORT)
        check = HcclCalculator([], CONFIG)
        communication_data = check._merge_hccl_ops_and_tasks(hccl_ops, hccl_tasks)
        self.assertEqual(2, len(communication_data))
        ProfilingScene().set_mode(ExportMode.ALL_EXPORT)

    def test_merge_hccl_ops_and_tasks_should_return_2_data_when_all_export_and_ops_queue_not_empty(self):
        hccl_ops = [HcclOps(model_id=4294967295), HcclOps(model_id=4294967296)]
        hccl_tasks = [HcclTask(model_id=4294967295), HcclTask(model_id=4294967296)]
        check = HcclCalculator([], CONFIG)
        communication_data = check._merge_hccl_ops_and_tasks(hccl_ops, hccl_tasks)
        self.assertEqual(2, len(communication_data))

    def test_get_hccl_op_report_data_should_return_empty_data_when_input_empty(self):
        hccl_data = []
        check = HcclCalculator([], CONFIG)
        hccl_op_report_data = check._get_hccl_op_report_data(hccl_data)
        self.assertEqual(hccl_op_report_data, {})

    def test_create_report_should_return_empty_when_input_is_empty(self):
        task_data = []
        check = HcclCalculator([], CONFIG)
        check._create_report(task_data)
        hccl_op_report_data = check._hccl_op_report_data
        self.assertEqual(hccl_op_report_data, [])

    def test_judge_calculate_again_should_return_true_in_operator_scene(self):
        scene = ProfilingScene()
        scene._scene = Constant.SINGLE_OP
        check = HcclCalculator([], CONFIG)
        self.assertTrue(check._judge_calculate_again())
        scene._scene = None

    def test_judge_calculate_again_should_return_false_when_tables_in_db(self):
        scene = ProfilingScene()
        scene._scene = Constant.SINGLE_OP
        check = HcclCalculator([], CONFIG)
        with mock.patch(NAMESPACE + ".DBManager.check_tables_in_db", return_value=True):
            self.assertFalse(check._judge_calculate_again())
        scene._scene = None

    def test_judge_calculate_again_should_return_true_when_tables_not_in_db(self):
        scene = ProfilingScene()
        scene._scene = Constant.STEP_INFO
        check = HcclCalculator([], CONFIG)
        ProfilingScene().set_mode(ExportMode.GRAPH_EXPORT)
        with mock.patch(NAMESPACE + ".DBManager.check_tables_in_db", return_value=False):
            self.assertTrue(check._judge_calculate_again())
        scene._scene = None
        ProfilingScene().set_mode(ExportMode.ALL_EXPORT)

    def test_ms_run(self):
        with mock.patch("os.path.exists", return_value=True), \
            mock.patch(NAMESPACE + ".HcclCalculator._judge_calculate_again", return_value=True), \
            mock.patch(NAMESPACE + ".HcclCalculator._drop_table"), \
            mock.patch(NAMESPACE + ".HcclCalculator.calculate"), \
            mock.patch(NAMESPACE + ".HcclCalculator.save"):
            check = HcclCalculator([], CONFIG)
            check.ms_run()

    def test_save_should_return_none_when_hccl_data_empty(self):
        check = HcclCalculator([], CONFIG)
        check._hccl_data = []
        self.assertIsNone(check.save())

    def test_save_should_return_none_when_hccl_data_not_empty_and_hccl_op_report_data_empty(self):
        with mock.patch("msmodel.hccl.hccl_model.HcclViewModel.rebuild_hccl_table"), \
             mock.patch("msmodel.hccl.hccl_model.HcclViewModel.insert_data_to_db"):
            check = HcclCalculator([], CONFIG)
            check._hccl_data = [HcclOps(model_id=4294967295)]
            check._hccl_op_report_data = []
            self.assertIsNone(check.save())

    def test_save_should_not_return_none_when_hccl_data_and_hccl_op_report_data_not_empty(self):
        with mock.patch("msmodel.hccl.hccl_model.HcclViewModel.rebuild_hccl_table"), \
             mock.patch("msmodel.hccl.hccl_model.HcclViewModel.insert_data_to_db"), \
             mock.patch("msmodel.hccl.hccl_model.HcclViewModel.rebuild_hccl_op_report_table"), \
             mock.patch("msmodel.hccl.hccl_model.HcclViewModel.insert_data_to_db"):
            check = HcclCalculator([], CONFIG)
            check._hccl_data = [HcclOps(model_id=4294967295)]
            check._hccl_op_report_data = [("all_reduce", 1.0, 1.0, 1.0, 1.0, 1.0, 100.0)]
            check.save()
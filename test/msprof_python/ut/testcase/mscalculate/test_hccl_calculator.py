#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023. All rights reserved.
import os
import unittest
from unittest import mock

from constant.constant import clear_dt_project
from mscalculate.hccl_calculator import HcclCalculator
from profiling_bean.db_dto.hccl_dto import HcclDto
from constant.constant import CONFIG
from common_func.constant import Constant


NAMESPACE = 'mscalculate.hccl_calculator'


class TestHcclCalculator(unittest.TestCase):
    DIR_PATH = os.path.join(os.path.dirname(__file__), 'DT_HcclCalculator')

    @staticmethod
    def construct_hccl_dto(op_name, timestamp=123456, duration=0, op_type="HCCL_OP_TYPE", connection_id=0):
        hccl_data = HcclDto()
        hccl_data.op_name, hccl_data.iteration, hccl_data.duration, hccl_data.first_timestamp, hccl_data.is_dynamic, \
        hccl_data.model_id, hccl_data.index_id, hccl_data.task_type, hccl_data.op_type, hccl_data.connection_id = \
            (op_name, 1, duration, timestamp, 0, 1, 1, "HCCL", op_type, connection_id)
        return hccl_data

    def setUp(self) -> None:
        os.makedirs(os.path.join(self.DIR_PATH, 'PROF1', 'device_0'))

    def tearDown(self) -> None:
        clear_dt_project(self.DIR_PATH)

    def test_calculate_should_return_empty_when_table_is_empty(self):
        with mock.patch(NAMESPACE + ".HcclViewModel.check_table", return_value=False):
            check = HcclCalculator([], CONFIG)
            check.calculate()
            hccl_data = check._hccl_data
            hccl_op_report_data = check._hccl_op_report_data
            self.assertEqual(hccl_data, [])
            self.assertEqual(hccl_op_report_data, [])

    def test_calculate_should_return_both_hccl_data_and_hccl_op_report_data_when_op_type_valid(self):
        with mock.patch(NAMESPACE + ".DBManager.check_tables_in_db", return_value=True), \
             mock.patch(NAMESPACE + '.HcclViewModel.get_hccl_communication_data', return_value=[
                 self.construct_hccl_dto("hccl_op", timestamp=1, duration=1, op_type="all_reduce")]):
            check = HcclCalculator([], CONFIG)
            check.calculate()
            hccl_data = check._hccl_data
            hccl_op_report_data = check._hccl_op_report_data
            self.assertEqual(15, len(hccl_data[0]))
            self.assertEqual([("all_reduce", 1.0, 1.0, 1.0, 1.0, 1.0, 100.0)], hccl_op_report_data)

    def test_calculate_should_return_only_hccl_data_not_valid_op_type(self):
        with mock.patch(NAMESPACE + ".DBManager.check_tables_in_db", return_value=True), \
             mock.patch(NAMESPACE + '.HcclViewModel.get_hccl_communication_data', return_value=[
                 self.construct_hccl_dto("hccl_op", timestamp=1, duration=1, op_type=Constant.NA)]):
            check = HcclCalculator([], CONFIG)
            check.calculate()
            hccl_data = check._hccl_data
            hccl_op_report_data = check._hccl_op_report_data
            self.assertEqual(15, len(hccl_data[0]))
            self.assertEqual([], hccl_op_report_data)

    def test_generate_hccl_op_info_should_return_three_data_when_the_input_len_is_three(self):
        hccl_data = [
            self.construct_hccl_dto("hccl_op1", connection_id=0),
            self.construct_hccl_dto("hccl_op1", timestamp=123457, connection_id=1),
            self.construct_hccl_dto("hccl_op2", timestamp=123458, connection_id=2)
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

    def test_get_hccl_op_data_should_return_dict_data_when_input_is_hccldto_list(self):
        hccl_data = [
            self.construct_hccl_dto("hccl_op1", timestamp=1, duration=2, op_type="all_reduce", connection_id=0),
            self.construct_hccl_dto("hccl_op2", timestamp=5, duration=3, op_type="all_reduce", connection_id=1),
            self.construct_hccl_dto("hccl_op3", timestamp=10, duration=2, op_type="all_gather", connection_id=2),
            self.construct_hccl_dto("hccl_op4", timestamp=15, duration=3, op_type="all_gather", connection_id=3)
        ]
        check = HcclCalculator([], CONFIG)
        hccl_op_report_data = check._get_hccl_op_report_data(hccl_data)
        self.assertEqual(hccl_op_report_data,
                         {"all_reduce": {"count": 2, "total_time": 5, "max": 3, "min": 2, "avg": 2.5},
                          "all_gather": {"count": 2, "total_time": 5, "max": 3, "min": 2, "avg": 2.5}
                          })

    def test_get_hccl_op_data_should_return_empty_data_when_input_empty(self):
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

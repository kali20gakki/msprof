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


NAMESPACE = 'mscalculate.hccl_calculator'


class TestHcclCalculator(unittest.TestCase):
    DIR_PATH = os.path.join(os.path.dirname(__file__), 'DT_HcclCalculator')

    @staticmethod
    def construct_hccl_dto(op_name, timestamp=123456):
        hccl_data = HcclDto()
        hccl_data.op_name, hccl_data.iteration, hccl_data.first_timestamp, hccl_data.is_dynamic, \
        hccl_data.model_id, hccl_data.index_id, hccl_data.task_type, hccl_data.op_type = \
            (op_name, 1, timestamp, 0, 1, 1, "HCCL", "HCCL_OP_TYPE")
        return hccl_data

    def setUp(self) -> None:
        os.makedirs(os.path.join(self.DIR_PATH, 'PROF1', 'device_0'))

    def tearDown(self) -> None:
        clear_dt_project(self.DIR_PATH)

    def test_calculate_should_return_data_when_data_is_not_none(self):
        with mock.patch(NAMESPACE + ".HcclCalculator._get_hccl_data",
                        return_value=[[self.construct_hccl_dto("hccl_op")], [["all_reduce", 1, 1, 1, 1, 1]]]):
            check = HcclCalculator([], CONFIG)
            check.calculate()
            hccl_data = check._hccl_data
            hccl_op_report_data = check._hccl_op_report_data
            self.assertEqual(hccl_data[0][2], "hccl_op")
            self.assertEqual(hccl_op_report_data[0][0], "all_reduce")

    def test_get_hccl_data(self):
        with mock.patch('os.path.exists', return_value=False):
            check = HcclCalculator([], CONFIG)
            self.assertEqual(([], []), check._get_hccl_data())

        with mock.patch('os.path.exists', return_value=True):
            check = HcclCalculator([], CONFIG)
            self.assertEqual(([], []), check._get_hccl_data())

        with mock.patch('os.path.exists', return_value=True), \
                mock.patch(NAMESPACE + '.HcclViewModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + '.HcclViewModel.get_hccl_communication_data',
                           return_value=[[self.construct_hccl_dto("hccl_op")], [["all_reduce", 1, 1, 1, 1, 1]]]):
            check = HcclCalculator([], CONFIG)
            self.assertEqual("hccl_op", check._get_hccl_data()[0][0].op_name)
            self.assertEqual("all_reduce", check._get_hccl_data()[1][0][0])

    def test_generate_hccl_op_info_should_return_three_data_when_the_input_len_is_three(self):
        hccl_data = [
            self.construct_hccl_dto("hccl_op1"),
            self.construct_hccl_dto("hccl_op1", timestamp=123457),
            self.construct_hccl_dto("hccl_op2", timestamp=123458)
        ]
        check = HcclCalculator([], CONFIG)
        check._generate_hccl_op_info(hccl_data)
        self.assertEqual(3, len(check._hccl_data))

    def test_cal_total_should_return_5_when_input_is_the_follow_task_time(self):
        task_time = {
                     '1': {'op_type': '1', 'count': 2, 'duration': 4, 'min': 1, 'max': 3, 'avg': 2},
                     '2': {'op_type': '2', 'count': 1, 'duration': 1, 'min': 1, 'max': 1, 'avg': 1}
        }
        check = HcclCalculator([], CONFIG)
        _cal_total = getattr(check, "_cal_total")
        result = _cal_total(task_time)
        self.assertEqual(result, 5)

    def test_create_report_should_return_list_data_when_input_is_not_none(self):
        task_data = [
                     ["1", 2, 4, 1, 3, 2],
                     ["2", 1, 1, 1, 1, 1]
        ]
        check = HcclCalculator([], CONFIG)
        check._create_report(task_data)
        hccl_op_report_data = check._hccl_op_report_data
        self.assertEqual(hccl_op_report_data, [("1", 2, 4.0, 1.0, 3.0, 2.0, 80.0),
                                               ("2", 1, 1.0, 1.0, 1.0, 1.0, 20.0)])

    def test_create_report_should_return_none_when_input_is_none(self):
        task_data = []
        check = HcclCalculator([], CONFIG)
        check._create_report(task_data)
        hccl_op_report_data = check._hccl_op_report_data
        self.assertEqual(hccl_op_report_data, [])



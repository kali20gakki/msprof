#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023. All rights reserved.
import os
import unittest
from unittest import mock

from common_func.ms_constant.str_constant import StrConstant
from constant.constant import clear_dt_project
from mscalculate.hccl_calculator import HcclCalculator
from profiling_bean.db_dto.hccl_dto import HcclDto

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

    def test_calculate(self):
        with mock.patch(NAMESPACE + ".HcclCalculator._get_hccl_data",
                        return_value=[self.construct_hccl_dto("hccl_op")]), \
                mock.patch(NAMESPACE + ".HcclCalculator._generate_hccl_op_info"):
            check = HcclCalculator([], {
                StrConstant.SAMPLE_CONFIG_PROJECT_PATH: os.path.join(self.DIR_PATH, 'PROF1', 'device_0')})
            check.calculate()

    def test_get_hccl_data(self):
        with mock.patch('os.path.exists', return_value=False):
            check = HcclCalculator([], {
                StrConstant.SAMPLE_CONFIG_PROJECT_PATH: os.path.join(self.DIR_PATH, 'PROF1', 'device_0')})
            self.assertEqual([], check._get_hccl_data())

        with mock.patch('os.path.exists', return_value=True):
            check = HcclCalculator([], {
                StrConstant.SAMPLE_CONFIG_PROJECT_PATH: os.path.join(self.DIR_PATH, 'PROF1', 'device_0')})
            self.assertEqual([], check._get_hccl_data())

        with mock.patch('os.path.exists', return_value=True), \
                mock.patch(NAMESPACE + '.HcclViewModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + '.HcclViewModel.get_hccl_communication_data',
                           return_value=[self.construct_hccl_dto("hccl_op")]):
            check = HcclCalculator([], {
                StrConstant.SAMPLE_CONFIG_PROJECT_PATH: os.path.join(self.DIR_PATH, 'PROF1', 'device_0')})
            self.assertEqual("hccl_op", check._get_hccl_data()[0].op_name)

    def test_generate_hccl_op_info(self):
        hccl_data = [
            self.construct_hccl_dto("hccl_op1"),
            self.construct_hccl_dto("hccl_op1", timestamp=123457),
            self.construct_hccl_dto("hccl_op2", timestamp=123458)
        ]
        check = HcclCalculator([], {
            StrConstant.SAMPLE_CONFIG_PROJECT_PATH: os.path.join(self.DIR_PATH, 'PROF1', 'device_0')})
        check._generate_hccl_op_info(hccl_data)
        self.assertEqual(3, len(check._hccl_data))

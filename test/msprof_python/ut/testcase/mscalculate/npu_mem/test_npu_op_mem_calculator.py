#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2022. All rights reserved.
import logging
import os
import unittest
from unittest import mock

from constant.constant import CONFIG
from msconfig.config_manager import ConfigManager
from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from constant.constant import clear_dt_project
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.hash_dict_constant import HashDictData
from profiling_bean.db_dto.npu_op_mem_dto import NpuOpMemDto
from mscalculate.npu_mem.npu_op_mem_calculator import NpuOpMemCalculator
from mscalculate.interface.icalculator import ICalculator

NAMESPACE = 'mscalculate.npu_mem.npu_op_mem_calculator'


class TestNpuOpMemCalculator(unittest.TestCase):

    def test_calculator_connect_db(self):
        check = NpuOpMemCalculator({}, CONFIG)
        check.calculator_connect_db()

    def test_calculate(self):
        with mock.patch(NAMESPACE + '.NpuOpMemCalculator._calc_memory_record'), \
                mock.patch(NAMESPACE + '.NpuOpMemCalculator._calc_operator_memory'), \
                mock.patch(NAMESPACE + '.NpuOpMemModel.get_table_data'):
            check = NpuOpMemCalculator({}, CONFIG)

    def test_save(self):
        with mock.patch(NAMESPACE + '.NpuOpMemModel.clear'), \
                mock.patch(NAMESPACE + '.NpuOpMemModel.init'), \
                mock.patch(NAMESPACE + '.NpuOpMemModel.flush'), \
                mock.patch(NAMESPACE + '.NpuOpMemModel.finalize'):
            check = NpuOpMemCalculator({}, CONFIG)
            setattr(check, "_memory_record", ['GE', '1', 0, 1, 'NPU:0'])
            result = check.save()
        with mock.patch(NAMESPACE + '.NpuOpMemModel.clear'), \
                mock.patch(NAMESPACE + '.NpuOpMemModel.init'), \
                mock.patch(NAMESPACE + '.NpuOpMemModel.flush'), \
                mock.patch(NAMESPACE + '.NpuOpMemModel.finalize'):
            check = NpuOpMemCalculator({}, CONFIG)
            setattr(check, "_opeartor_memory", ['a', 1, '0', '1', '1', 0, 1, 0, 1, "NPU:0", "a"])
            result = check.save()

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.NpuOpMemCalculator.calculator_connect_db'), \
                mock.patch(NAMESPACE + '.NpuOpMemCalculator.calculate'), \
                mock.patch(NAMESPACE + '.NpuOpMemCalculator.save'):
            check = NpuOpMemCalculator({}, CONFIG)
            check.ms_run()

    def test_calc_operator_memory(self):
        check = NpuOpMemCalculator({}, CONFIG)

        npu_op_mem_dto_1 = NpuOpMemDto()
        npu_op_mem_dto_1.operator = '1'
        npu_op_mem_dto_1.addr = 100
        npu_op_mem_dto_1.size = 1
        npu_op_mem_dto_1.timestamp = 111
        npu_op_mem_dto_1.thread_id = 312
        npu_op_mem_dto_1.total_allocate_memory = 0
        npu_op_mem_dto_1.total_reserve_memory = 1
        npu_op_mem_dto_1.level = 10000
        npu_op_mem_dto_1.type = 6
        npu_op_mem_dto_1.device_type = "NPU:0"

        npu_op_mem_dto_2 = NpuOpMemDto()
        npu_op_mem_dto_2.operator = '1'
        npu_op_mem_dto_2.addr = 100
        npu_op_mem_dto_2.size = -1
        npu_op_mem_dto_2.timestamp = 112
        npu_op_mem_dto_2.thread_id = 312
        npu_op_mem_dto_2.total_allocate_memory = 0
        npu_op_mem_dto_2.total_reserve_memory = 1
        npu_op_mem_dto_2.level = 10000
        npu_op_mem_dto_2.type = 6
        npu_op_mem_dto_2.device_type = "NPU:0"

        setattr(check, "_op_data", [npu_op_mem_dto_1, npu_op_mem_dto_2])
        setattr(check, "_opeartor_memory", [])
        check._calc_operator_memory()
        self.assertEqual(check._opeartor_memory, [['1', 1, 111, 112, 1, 1, 1, 0, 1, "NPU:0", '']])

    def test_calc_memory_record(self):
        check = NpuOpMemCalculator({}, CONFIG)

        npu_op_mem_dto_1 = NpuOpMemDto()
        npu_op_mem_dto_1.operator = '1'
        npu_op_mem_dto_1.addr = 100
        npu_op_mem_dto_1.size = 1
        npu_op_mem_dto_1.timestamp = 111
        npu_op_mem_dto_1.thread_id = 312
        npu_op_mem_dto_1.total_allocate_memory = 0
        npu_op_mem_dto_1.total_reserve_memory = 1
        npu_op_mem_dto_1.level = 10000
        npu_op_mem_dto_1.type = 6
        npu_op_mem_dto_1.device_type = "NPU:0"

        npu_op_mem_dto_2 = NpuOpMemDto()
        npu_op_mem_dto_2.operator = '1'
        npu_op_mem_dto_2.addr = 100
        npu_op_mem_dto_2.size = -1
        npu_op_mem_dto_2.timestamp = 112
        npu_op_mem_dto_2.thread_id = 312
        npu_op_mem_dto_2.total_allocate_memory = 0
        npu_op_mem_dto_2.total_reserve_memory = 1
        npu_op_mem_dto_2.level = 10000
        npu_op_mem_dto_2.type = 6
        npu_op_mem_dto_2.device_type = "NPU:0"

        setattr(check, "_op_data", [npu_op_mem_dto_1, npu_op_mem_dto_2])
        setattr(check, "_memory_record", [[]])
        check._calc_memory_record()
        self.assertEqual(check._memory_record, [['GE', 111, 1, 0, "NPU:0"], ['GE', 112, 1, 0, "NPU:0"]])

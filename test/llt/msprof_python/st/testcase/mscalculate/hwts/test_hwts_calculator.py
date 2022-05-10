#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import os
import shutil
import unittest
from unittest import mock

from analyzer.scene_base.profiling_scene import ProfilingScene
from mscalculate.hwts.hwts_calculator import HwtsCalculator
from profiling_bean.prof_enum.data_tag import DataTag
from framework.offset_calculator import FileCalculator
from framework.offset_calculator import OffsetCalculator
from constant.constant import CONFIG
from common_func.constant import Constant

NAMESPACE = 'mscalculate.hwts.hwts_calculator'


class TestHwtsCalculator(unittest.TestCase):
    HWTS_LOG_SIZE = 64
    FILE_0 = 'hwts.data.0.slice_0'
    FILE_1 = 'hwts.data.0.slice_1'

    def setUp(self):
        self.current_path = "./hwts_calculator_test/"
        self.file_folder = "data/"
        self.file_dict = {
            self.FILE_0:b'\xc0\x00\xd3k\x00\x00\x02\x00\xed\xb9^I\xdc\x06\x00\x00\x05\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xd1\x00\xd3k\x00\x00\x02\x00/\xbe^I\xdc\x06\x00\x00\x05\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xe0\x00\xd3k\x00\x00\x03\x00\xb5\x04_I\xdc\x06\x00\x00\x05\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xf1\x00\xd3k\x00\x00\x03\x00s\x06_I\xdc\x06\x00\x00\x05\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xd3k\x00\x00\x04\x00I?_I\xdc\x06\x00\x00\x05\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x11\x00\xd3k\x00\x00\x04\x00\xdd@_I\xdc\x06\x00\x00\x05\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00 \x00\xd3k\x00\x00\x05\x00\xc6z_I\xdc\x06\x00\x00\x05\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x001\x00\xd3k\x00\x00\x05\x00\x87|_I\xdc\x06\x00\x00\x05\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00',
            self.FILE_1:b'@\x00\xd3k\x00\x00\x03\x00wn\x7f]\xdc\x06\x00\x00\x02\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00Q\x00\xd3k\x00\x00\x03\x00\x94t\x7f]\xdc\x06\x00\x00\x02\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
        }

        if not os.path.exists(self.current_path):
            os.mkdir(self.current_path)
        if not os.path.exists(self.current_path + self.file_folder):
            os.mkdir(self.current_path + self.file_folder)

        for file_name, file_content in self.file_dict.items():
            with open(self.current_path + self.file_folder + file_name, 'wb') as _file_writer:
                _file_writer.write(file_content)

        self.file_list = [self.FILE_0, self.FILE_1]
        file_dict = {DataTag.HWTS: self.file_list}
        sample_config = {
            'result_dir': self.current_path, 'device_id': '0', 'iter_id': 1,
            'job_id': 'job_default', 'ip_address': '127.0.0.1', 'model_id': -1
        }
        self.calculator = HwtsCalculator(file_dict, sample_config)

    def tearDown(self):
        if os.path.exists(self.current_path):
            shutil.rmtree(self.current_path)

    def test_ms_run_0(self):
        expect_res = 10
        offset_calculator = OffsetCalculator(self.file_list, self.HWTS_LOG_SIZE, self.current_path)

        ProfilingScene()._scene = "single_op"
        with mock.patch(NAMESPACE + '.HwtsCalculator.save'), \
                mock.patch('framework.offset_calculator.OffsetCalculator', return_value=offset_calculator):
            self.calculator.ms_run()
            self.assertEqual(expect_res, len(self.calculator._log_data))

    def test_ms_run_1(self):
        expect_res = 8

        offset_count = 0
        total_count = 8
        file_calculator = FileCalculator(self.file_list, self.HWTS_LOG_SIZE, self.current_path,
                                          offset_count, total_count)

        ProfilingScene()._scene = "step_info"
        with mock.patch(NAMESPACE + '.HwtsCalculator.save'), \
                mock.patch('model.iter_rec.iter_rec_model.HwtsIterModel.check_db', return_value=True), \
                mock.patch('model.iter_rec.iter_rec_model.HwtsIterModel.check_table', return_value=True), \
                mock.patch('common_func.msprof_iteration.MsprofIteration.get_iteration_id_by_index_id', return_value=1), \
                mock.patch('model.iter_rec.iter_rec_model.HwtsIterModel.get_task_offset_and_sum', return_value=(offset_count, total_count)), \
                mock.patch('framework.offset_calculator.FileCalculator', return_value=file_calculator):
            self.calculator.ms_run()
            self.assertEqual(expect_res, len(self.calculator._log_data))

    def test_add_batch_id(self):
        expect_single_res = [[1, 2, 0], [3, 4, 0]]
        prep_data_res = [[1, 2], [3, 4]]

        ProfilingScene().init("")
        check = HwtsCalculator({}, CONFIG)

        ProfilingScene()._scene = Constant.SINGLE_OP
        res = check._add_batch_id(prep_data_res)
        self.assertEqual(res, expect_single_res)

#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import sqlite3
import struct
import unittest
from unittest import mock

import pytest
from common_func.msprof_exception import ProfException
from msparser.iter_rec.iter_rec_parser import IterRecParser
from profiling_bean.prof_enum.data_tag import DataTag

from constant.constant import CONFIG

NAMESPACE = 'msparser.iter_rec.iter_rec_parser'


class TestIterRecParser(unittest.TestCase):
    file_list = {DataTag.HWTS: ['hwts.data.0.slice_0']}

    def test_get_iter_end_dict(self):
        pass

    def test_is_ai_core_task(self):
        with mock.patch('common_func.msprof_iteration.MsprofIteration.get_iteration_end_dict', return_value={1: 101}):
            check = IterRecParser(self.file_list, CONFIG)
            check._ge_op_iter_dict = {"100": ("100-3",)}
            check._iter_recorder.set_current_iter_id(100)
            check._is_ai_core_task('100', '3', 1)

    def test_set_current_iter_id(self):
        with mock.patch('common_func.msprof_iteration.MsprofIteration.get_iteration_end_dict', return_value={1: 101}):
            check = IterRecParser(self.file_list, CONFIG)
            check._iter_recorder.set_current_iter_id(100)
        with mock.patch(NAMESPACE + '.logging.error'), \
             mock.patch('common_func.msprof_iteration.MsprofIteration.get_iteration_end_dict', return_value={}):
            with pytest.raises(ProfException) as err:
                check = IterRecParser(self.file_list, CONFIG)
                check._iter_recorder.set_current_iter_id(100)
            self.assertEqual(err.value.code, 8)

    def test_read_hwts_data(self):
        with mock.patch(NAMESPACE + '.IterRecParser._is_ai_core_task', return_value=True), \
             mock.patch('common_func.msprof_iteration.MsprofIteration.get_iteration_end_dict', return_value={1: 101}):
            data = struct.pack("=BBHHHQ12I", 1, 2, 3, 4, 5, 6, 7, 8, 9, 7, 8, 9, 5, 3, 2, 5, 4, 6)
            check = IterRecParser(self.file_list, CONFIG)
            check._read_hwts_data(bytes(data))

    def test_parse_hwts_data(self):
        data = struct.pack("=BBHHHQ12I", 1, 2, 3, 4, 5, 6, 7, 8, 9, 7, 8, 9, 5, 3, 2, 5, 4, 6)
        with mock.patch(NAMESPACE + '.PathManager.get_data_file_path', return_value='test'), \
             mock.patch(NAMESPACE + '.logging.info'), \
             mock.patch(NAMESPACE + '.IterRecParser._read_hwts_data'), \
             mock.patch('common_func.file_manager.check_path_valid'), \
             mock.patch('os.path.getsize', return_value=len(data)), \
             mock.patch('builtins.open', mock.mock_open(read_data=data)), \
             mock.patch('common_func.msprof_iteration.MsprofIteration.get_iteration_end_dict', return_value={1: 101}):
            check = IterRecParser(self.file_list, CONFIG)
            check._parse_hwts_data()
            check_1 = IterRecParser({}, CONFIG)
            check_1._parse_hwts_data()

    def test_parse(self):
        with mock.patch('model.ge.ge_info_calculate_model.GeInfoModel.check_db', return_value=True), \
             mock.patch('model.ge.ge_info_calculate_model.GeInfoModel.check_table', return_value=True), \
             mock.patch('model.ge.ge_info_calculate_model.GeInfoModel.get_ge_data', return_value={}), \
             mock.patch('model.ge.ge_info_calculate_model.GeInfoModel.finalize'), \
             mock.patch(NAMESPACE + '.IterRecParser._parse_hwts_data'), \
             mock.patch(NAMESPACE + '.logging.error'), \
             mock.patch('common_func.msprof_iteration.MsprofIteration.get_iteration_end_dict', return_value={1: 101}), \
             mock.patch('model.ge.ge_info_calculate_model.GeInfoModel.get_batch_dict', return_value={}):
            check = IterRecParser(self.file_list, CONFIG)
            check.parse()

    def test_save(self):
        with mock.patch('model.iter_rec.iter_rec_model.HwtsIterModel.init'), \
             mock.patch('model.iter_rec.iter_rec_model.HwtsIterModel.flush'), \
             mock.patch('model.iter_rec.iter_rec_model.HwtsIterModel.finalize', side_effect=sqlite3.Error), \
             mock.patch(NAMESPACE + '.Utils.obj_list_to_list'), \
             mock.patch(NAMESPACE + '.logging.error'), \
             mock.patch('common_func.msprof_iteration.MsprofIteration.get_iteration_end_dict', return_value={1: 101}):
            check = IterRecParser(self.file_list, CONFIG)
            check._iter_info_dict = {'a': 'b'}
            check.save()

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.IterRecParser.parse'), \
             mock.patch(NAMESPACE + '.IterRecParser.save', side_effect=ProfException(0)), \
             mock.patch(NAMESPACE + '.logging.warning'), \
             mock.patch('common_func.msprof_iteration.MsprofIteration.get_iteration_end_dict', return_value={1: 101}):
            check = IterRecParser(self.file_list, CONFIG)
            check.ms_run()

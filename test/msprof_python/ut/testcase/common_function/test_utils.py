#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.import unittest
import unittest
from unittest import mock

import pytest

from common_func.utils import Utils


class TestUtils(unittest.TestCase):

    def test_is_valid_num(self):
        res = Utils.is_valid_num(None)
        self.assertEqual(res, 0)

    def test_percentile(self):
        data = [1, 2, 3, 4, 5, 6, 7, 8]
        ret = Utils.percentile(data, 0.33)
        self.assertEqual(3, ret)

    def test_is_step_scene_should_return_false_when_not_table_in_db(self):
        with mock.patch('msmodel.interface.base_model.BaseModel.check_table', return_value=False):
            self.assertFalse(Utils.is_step_scene('./'))

    def test_is_step_scene_should_return_true_when_step_trace_data_has_value_other_than_4294967295(self):
        with mock.patch('msmodel.interface.base_model.BaseModel.check_table', return_value=True), \
                mock.patch('common_func.db_manager.DBManager.fetch_one_data', return_value=(1, )), \
                mock.patch('msmodel.interface.base_model.BaseModel.finalize'):
            self.assertTrue(Utils.is_step_scene('./'))

    def test_generator_to_list_should_return_empty_list_when_input_None_throws_exception(self):
        self.assertEqual([], Utils.generator_to_list(None))

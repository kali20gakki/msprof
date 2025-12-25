#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2024. All rights reserved.
import unittest
from unittest import mock

from msmodel.stars.low_power_model import LowPowerModel
from msmodel.stars.low_power_model import LowPowerViewModel

NAMESPACE = 'msmodel.stars.low_power_model'


class TestLowPowerModel(unittest.TestCase):

    def test_flush(self):
        with mock.patch(NAMESPACE + '.LowPowerModel.insert_data_to_db'):
            check = LowPowerModel('test', 'test', [])
            check.flush([])


class TestLowPowerViewModel(unittest.TestCase):

    def test_get_timeline_data_should_return_data(self):
        with mock.patch('msmodel.interface.base_model.DBManager.judge_table_exist', return_value=True), \
                mock.patch('msmodel.interface.base_model.DBManager.fetch_all_data', return_value=[1]):
            check = LowPowerViewModel('test', 'test', [])
            res = check.get_timeline_data()
        self.assertEqual(res, [1])

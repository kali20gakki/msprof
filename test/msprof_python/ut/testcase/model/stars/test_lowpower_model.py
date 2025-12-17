# -------------------------------------------------------------------------
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
# This file is part of the MindStudio project.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#    http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------
import unittest
from unittest import mock

from msmodel.stars.lowpower_model import LowPowerModel

NAMESPACE = 'msmodel.stars.lowpower_model'


class TestInterSocModel(unittest.TestCase):

    def test_flush(self):
        with mock.patch(NAMESPACE + '.LowPowerModel.insert_data_to_db'):
            check = LowPowerModel('test', 'test', [])
            check.flush({})

    def test_get_timeline_data(self):
        with mock.patch('msmodel.interface.base_model.DBManager.judge_table_exist', return_value=True), \
             mock.patch('msmodel.interface.base_model.DBManager.fetch_all_data', return_value=[1]):
            check = LowPowerModel('test', 'test', [])
            res = check.get_timeline_data()
        self.assertEqual(res, [1])

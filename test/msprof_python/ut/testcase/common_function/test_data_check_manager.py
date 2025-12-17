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

from common_func.data_check_manager import DataCheckManager

NAMESPACE = 'common_func.data_check_manager'


class TestDataCheckManager(unittest.TestCase):

    def test_check_file_size(self):
        check = DataCheckManager._check_file_size("a.done", "b")
        self.assertEqual(check, False)

        with mock.patch('os.path.realpath', return_value="a"), \
             mock.patch('os.path.getsize', return_value=0):
            check = DataCheckManager._check_file_size("a", "b")
        self.assertEqual(check, False)

        with mock.patch('os.path.realpath', return_value="a"), \
             mock.patch('os.path.getsize', return_value=10):
            check = DataCheckManager._check_file_size("a", "b")
        self.assertEqual(check, True)

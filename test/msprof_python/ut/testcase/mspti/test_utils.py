#!/usr/bin/env python
# coding=utf-8
"""
function: test mspti constant
Copyright Huawei Technologies Co., Ltd. 2020-2024. All rights reserved.
"""

import unittest
from unittest import mock


class TestMsptiUtils(unittest.TestCase):

    def test_log_function(self):
        with mock.patch('importlib.import_module'):
            from mspti import utils
            utils.print_info_msg("info msg")
            utils.print_warn_msg("warn msg")
            utils.print_error_msg("error msg")


if __name__ == '__main__':
    unittest.main()

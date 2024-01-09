#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
from argparse import Namespace
from unittest import mock
import unittest

from msinterface.msprof_entrance import MsprofEntrance
from common_func.profiling_scene import ProfilingScene

NAMESPACE = 'msinterface.msprof_entrance'


class TestMsprofEntrance(unittest.TestCase):

    def test_main(self):
        args = ["msprof_entrance.py", "query", "-dir", "test"]
        collection_path = {"collection_path": "test"}
        args_co = Namespace(**collection_path)
        with mock.patch('sys.argv', args), \
                mock.patch('argparse.ArgumentParser.parse_args', return_value=args_co), \
                mock.patch(NAMESPACE + '.QueryCommand.process'), \
                mock.patch(NAMESPACE + '.check_path_valid'), \
                mock.patch('sys.exit'):
            MsprofEntrance().main()

    def test_main_should_set_all_export_false_when_set_model_id_and_iteration_id(self):
        args = ["msprof.py", "export", "summary", "-dir", "test", "--model-id", "1", "--iteration-id", "2"]
        with mock.patch('sys.argv', args), \
                mock.patch(NAMESPACE + '.check_path_valid'), \
                mock.patch(NAMESPACE + '.ExportCommand.process'), \
                mock.patch('sys.exit'):
            MsprofEntrance().main()
            self.assertFalse(ProfilingScene().is_all_export())

    def test_main_should_print_error_when_set_model_id_but_no_iteration_id(self):
        args = ["msprof.py", "export", "summary", "-dir", "test", "--model-id", "1"]
        with mock.patch('sys.argv', args), \
                mock.patch('common_func.common.error'), \
                mock.patch(NAMESPACE + '.check_path_valid'), \
                mock.patch(NAMESPACE + '.ExportCommand.process'), \
                mock.patch('sys.exit'):
            MsprofEntrance().main()

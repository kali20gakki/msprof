#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import unittest
from argparse import Namespace
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from common_func.profiling_scene import ExportMode
from common_func.profiling_scene import ProfilingScene
from msinterface.msprof_entrance import MsprofEntrance

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

    def test_main_should_raise_system_exit_when_no_argument(self):
        args = ["msprof.py"]
        with mock.patch('sys.argv', args), \
                mock.patch('common_func.common.error'), \
                mock.patch(NAMESPACE + '.check_path_valid'), \
                mock.patch(NAMESPACE + '.check_path_char_valid'), \
                mock.patch(NAMESPACE + '.ExportCommand.process'):
            with self.assertRaises(SystemExit):
                MsprofEntrance().main()

    def test_main_should_raise_system_exit_when_no_argument_with_export(self):
        args = ["msprof.py", "export"]
        with mock.patch('sys.argv', args), \
                mock.patch('common_func.common.error'), \
                mock.patch(NAMESPACE + '.check_path_valid'), \
                mock.patch(NAMESPACE + '.check_path_char_valid'), \
                mock.patch(NAMESPACE + '.ExportCommand.process'):
            with self.assertRaises(SystemExit):
                MsprofEntrance().main()

    def test_main_should_raise_system_exit_when_dir_len_is_over_1024(self):
        args = ["msprof.py", "export", "summary", "-dir", "t"*1025]
        with mock.patch('sys.argv', args), \
                mock.patch('common_func.common.error'), \
                mock.patch(NAMESPACE + '.check_path_valid'), \
                mock.patch(NAMESPACE + '.check_path_char_valid'), \
                mock.patch(NAMESPACE + '.ExportCommand.process'):
            with self.assertRaises(SystemExit):
                MsprofEntrance().main()

    def test_set_export_mode(self):
        args_dic = {"collection_path": "test", "iteration_id": 1, "model_id": 4294967295, "iteration_count": None}
        InfoConfReader()._info_json = {"drvVersion": 0}
        ProfilingScene().set_mode(ExportMode.STEP_EXPORT)
        args = Namespace(**args_dic)
        with mock.patch('sys.argv', args):
            MsprofEntrance()._set_export_mode(args)

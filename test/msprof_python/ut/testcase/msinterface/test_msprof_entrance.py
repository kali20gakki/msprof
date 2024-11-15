#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import os
import unittest
from argparse import Namespace, ArgumentTypeError
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from common_func.profiling_scene import ExportMode
from common_func.profiling_scene import ProfilingScene
from msinterface.msprof_entrance import MsprofEntrance

NAMESPACE = 'msinterface.msprof_entrance'


class TestMsprofEntrance(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        if os.path.exists("test_entrance_dir"):
            import shutil
            shutil.rmtree("test_entrance_dir")
        os.makedirs("test_entrance_dir/subdir", exist_ok=True)

    @classmethod
    def tearDownClass(cls):
        import shutil
        shutil.rmtree("test_entrance_dir")

    def test_main(self):
        args = ["msprof_entrance.py", "query", "-dir", "test_entrance_dir"]
        collection_path = {"collection_path": "test_entrance_dir"}
        args_co = Namespace(**collection_path)
        with mock.patch('sys.argv', args), \
                mock.patch('argparse.ArgumentParser.parse_args', return_value=args_co), \
                mock.patch(NAMESPACE + '.QueryCommand.process'), \
                mock.patch(NAMESPACE + '.check_path_valid'), \
                mock.patch('sys.exit'):
            MsprofEntrance().main()

    def test_main_should_set_all_export_false_when_set_model_id_and_iteration_id(self):
        args = ["msprof.py", "export", "summary", "-dir", "test_entrance_dir", "--model-id", "1", "--iteration-id", "2"]
        with mock.patch('sys.argv', args), \
                mock.patch(NAMESPACE + '.check_path_valid'), \
                mock.patch(NAMESPACE + '.ExportCommand.process'), \
                mock.patch('sys.exit'):
            MsprofEntrance().main()
            self.assertFalse(ProfilingScene().is_all_export())

    def test_main_should_print_error_when_set_model_id_but_no_iteration_id(self):
        args = ["msprof.py", "export", "summary", "-dir", "test_entrance_dir", "--model-id", "1"]
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
        args = ["msprof.py", "export", "summary", "-dir", "t" * 1025]
        with mock.patch('sys.argv', args), \
                mock.patch('common_func.common.error'), \
                mock.patch(NAMESPACE + '.check_path_valid'), \
                mock.patch(NAMESPACE + '.check_path_char_valid'), \
                mock.patch(NAMESPACE + '.ExportCommand.process'), \
                mock.patch(NAMESPACE + '.get_all_subdir'), \
                mock.patch(NAMESPACE + '.check_parent_dir_invalid', return_value=False):
            with self.assertRaises(SystemExit):
                MsprofEntrance().main()

    def test_main_should_success_when_analyze_args_is_valid(self):
        # 只能满足覆盖率要求，无法校验功能
        args = ["msprof.py", "analyze", "--rule", "communication", "--clear", "--type", "text", "-dir",
                "test_entrance_dir"]
        with mock.patch('sys.argv', args), \
                mock.patch(NAMESPACE + '.check_path_valid'), \
                mock.patch(NAMESPACE + '.ExportCommand.process'):
            with self.assertRaises(SystemExit):
                MsprofEntrance().main()

        args = [
            "msprof.py", "analyze", "--rule", "communication,communication_matrix",
            "--clear", "--type", "db", "-dir", "test_entrance_dir"
        ]
        with mock.patch('sys.argv', args), \
                mock.patch(NAMESPACE + '.check_path_valid'), \
                mock.patch(NAMESPACE + '.ExportCommand.process'):
            with self.assertRaises(SystemExit):
                MsprofEntrance().main()

    def test_set_export_mode(self):
        args_dic = {"collection_path": "test_entrance_dir", "iteration_id": 1, "model_id": 4294967295,
                    "iteration_count": None}
        InfoConfReader()._info_json = {"drvVersion": 0}
        ProfilingScene().set_mode(ExportMode.STEP_EXPORT)
        args = Namespace(**args_dic)
        with mock.patch('sys.argv', args):
            MsprofEntrance()._set_export_mode(args)

    def test_validate_analyze_rule_should_return_valid_str_when_str_is_valid(self):
        analyze_rule = "communication"
        self.assertEqual(analyze_rule, MsprofEntrance()._validate_analyze_rule(analyze_rule))

        analyze_rule = "communication_matrix"
        self.assertEqual(analyze_rule, MsprofEntrance()._validate_analyze_rule(analyze_rule))

        analyze_rule = "communication,communication_matrix"
        self.assertEqual(analyze_rule, MsprofEntrance()._validate_analyze_rule(analyze_rule))

    def test_validate_analyze_rule_should_raise_error_when_type_is_invalid(self):
        analyze_rule = "xxx,communication_matrix"
        with self.assertRaises(ArgumentTypeError):
            self.assertEqual(analyze_rule, MsprofEntrance()._validate_analyze_rule(analyze_rule))

        analyze_rule = "xxx"
        with self.assertRaises(ArgumentTypeError):
            self.assertEqual(analyze_rule, MsprofEntrance()._validate_analyze_rule(analyze_rule))


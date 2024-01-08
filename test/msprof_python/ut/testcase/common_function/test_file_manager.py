#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

import os
import shutil
import unittest
from unittest import mock

from common_func.constant import Constant
from common_func.file_manager import FileManager
from common_func.file_manager import check_db_path_valid
from common_func.file_name_manager import FileNameManagerConstant
from common_func.file_manager import check_path_valid


NAMESPACE = 'common_func.file_manager'


class TestFileManager(unittest.TestCase):

    def test_is_analyzed_data(self):
        os.mkdir("test_file_manager", 0o777)
        self.assertEqual(False, FileManager.is_analyzed_data("test_file_manager"))
        os.mkdir(os.path.join("test_file_manager", "data"), 0o777)
        self.assertEqual(False, FileManager.is_analyzed_data("test_file_manager"))
        with os.fdopen(os.open(os.path.join("test_file_manager/data", FileNameManagerConstant.ALL_FILE_TAG),
                               Constant.WRITE_FLAGS, Constant.WRITE_MODES), "w"):
            pass
        self.assertEqual(True, FileManager.is_analyzed_data("test_file_manager"))
        shutil.rmtree("test_file_manager")

    def test_check_path_vaild(self):
        with mock.patch("common_func.return_code_checker.ReturnCodeCheck.print_and_return_status"), \
                mock.patch("os.path.exists", return_value=False), \
                mock.patch("os.path.isfile", return_value=False), \
                mock.patch("os.path.getsize", return_value=2), \
                mock.patch("os.path.islink", return_value=False), \
                mock.patch("os.path.isdir", return_value=False):

            check_path_valid("test.txt", True, max_size=1)

    def test_check_db_path_vaild(self):
        with mock.patch("common_func.return_code_checker.ReturnCodeCheck.print_and_return_status"), \
                mock.patch("os.path.exists", return_value=True), \
                mock.patch("os.path.getsize", return_value=2):

            check_db_path_valid("test.txt", False, max_size=1)

    def test_check_path_valid_should_return_error_when_dir_is_link(self):
        with mock.patch("os.path.exists", return_value=True), \
             mock.patch('os.path.isdir', return_value=True), \
             mock.patch('os.path.islink', return_value=True), \
             mock.patch('os.access', return_value=True):
            with mock.patch('common_func.return_code_checker.ReturnCodeCheck.print_and_return_status') \
                    as mock_print:
                check_path_valid("/host/host", False, max_size=1)
                mock_print.assert_called_with(
                    '{"status": 1, "info": "The path \'/host/host\' is link. Please check the path."}')

    def test_check_path_valid_should_return_error_when_path_is_not_dir(self):
        with mock.patch("os.path.exists", return_value=True), \
             mock.patch('os.path.isdir', return_value=False), \
             mock.patch('os.access', return_value=True):
            with mock.patch('common_func.return_code_checker.ReturnCodeCheck.print_and_return_status') \
                    as mock_print:
                check_path_valid("/host/host", False, max_size=1)
                mock_print.assert_called_with(
                    '{"status": 1, "info": "The path \'/host/host\' is not a directory. Please check the path."}')

    def test_check_path_valid_should_return_error_when_dir_is_not_access(self):
        with mock.patch('os.path.exists', return_value=True), \
             mock.patch('os.path.isdir', return_value=True), \
             mock.patch('os.path.islink', return_value=False), \
             mock.patch('os.access', return_value=False):
            with mock.patch('common_func.return_code_checker.ReturnCodeCheck.print_and_return_status') \
                as mock_print:
                check_path_valid("/host/host", False, max_size=1)
                mock_print.assert_called_with(
                    '{"status": 1, "info": "The path \'/host/host\' does not have permission to read. '
                    'Please check that the path is readable."}')

    def test_check_path_valid_should_return_error_when_path_is_empty(self):
        with mock.patch('os.path.exists', return_value=True), \
             mock.patch('os.access', return_value=True):
            with mock.patch('common_func.return_code_checker.ReturnCodeCheck.print_and_return_status') \
                    as mock_print:
                check_path_valid("", False, max_size=1)
                mock_print.assert_any_call(
                    '{"status": 1, "info": "The path is empty. '
                    'Please enter a valid path."}')

    def test_check_path_valid_should_return_error_when_path_is_not_exist(self):
        with mock.patch('os.path.exists', return_value=False), \
             mock.patch('os.access', return_value=True):
            with mock.patch('common_func.return_code_checker.ReturnCodeCheck.print_and_return_status') \
                    as mock_print:
                check_path_valid("/host/host", False, max_size=1)
                mock_print.assert_any_call(
                    '{"status": 1, "info": "The path \'/host/host\' does not exist. '
                    'Please check that the path exists."}')

    def test_check_db_path_valid_should_return_error_when_path_is_link(self):
        with mock.patch('os.path.islink', return_value=True):
            with mock.patch('common_func.return_code_checker.ReturnCodeCheck.print_and_return_status') as mock_print:
                check_db_path_valid("/host/test.db", True, max_size=1)
                mock_print.assert_called_with(
                    '{"status": 1, "info": "The db file \'/host/test.db\' is link. Please check the path."}')

#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

import os
import shutil
import unittest
from unittest import mock

from common_func.constant import Constant
from common_func.empty_class import EmptyClass
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
                mock.patch("os.path.isdir", return_value=False), \
                mock.patch("os.path.islink", return_value=True):

            check_path_valid("test.txt", True, max_size=1)

    def test_check_db_path_vaild(self):
        with mock.patch("common_func.return_code_checker.ReturnCodeCheck.print_and_return_status"), \
                mock.patch("os.path.islink", return_value=True), \
                mock.patch("os.path.exists", return_value=True), \
                mock.patch("os.path.getsize", return_value=2):

            check_db_path_valid("test.txt", False, max_size=1)

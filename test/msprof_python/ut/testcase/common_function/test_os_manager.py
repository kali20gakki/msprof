#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

import unittest
from unittest import mock

from common_func.msprof_exception import ProfException
from common_func.os_manager import _check_path_valid


class TestOsManager(unittest.TestCase):
    def test_check_path_valid_should_return_error_when_is_file_and_path_not_exists(self):
        path = 'test.cpp'
        is_file = True
        with mock.patch('os.path.exists', return_value=False):
            with self.assertRaises(ProfException) as context:
                _check_path_valid(path, is_file)
        self.assertEqual(context.exception.code, ProfException.PROF_INVALID_PATH_ERROR)
        self.assertEqual(str(context.exception),
                         "The path \"test.cpp\" does not exist. Please check that the path exists.")

    def test_check_path_valid_should_return_error_when_is_not_file_and_path_not_exists(self):
        path = '/host/host'
        is_file = False
        with mock.patch('os.path.exists', return_value=False):
            with self.assertRaises(ProfException) as context:
                _check_path_valid(path, is_file)
        self.assertEqual(context.exception.code, ProfException.PROF_INVALID_PATH_ERROR)
        self.assertEqual(str(context.exception),
                         "The path \"/host/host\" does not exist. Please check that the path exists.")

    def test_check_path_valid_should_return_error_when_dir_path_is_empty(self):
        path = ''
        is_file = False
        with mock.patch('os.path.exists', return_value=True):
            with self.assertRaises(ProfException) as context:
                _check_path_valid(path, is_file)
        self.assertEqual(context.exception.code, ProfException.PROF_INVALID_PARAM_ERROR)
        self.assertEqual(str(context.exception),
                         "The path is empty. Please enter a valid path.")

    def test_check_path_valid_should_return_error_when_file_path_is_empty(self):
        path = ''
        is_file = True
        with mock.patch('os.path.exists', return_value=True):
            with self.assertRaises(ProfException) as context:
                _check_path_valid(path, is_file)
        self.assertEqual(context.exception.code, ProfException.PROF_INVALID_PARAM_ERROR)
        self.assertEqual(str(context.exception),
                         "The path is empty. Please enter a valid path.")

    def test_check_path_valid_should_return_error_when_is_file_and_path_is_not_a_file(self):
        path = '/path/host/test.so'
        is_file = True
        with mock.patch('os.path.exists', return_value=True), \
             mock.patch('os.path.isfile', return_value=False):
            with self.assertRaises(ProfException) as context:
                _check_path_valid(path, is_file)
        self.assertEqual(context.exception.code, ProfException.PROF_INVALID_PATH_ERROR)
        self.assertEqual(str(context.exception), "The path \"/path/host/test.so\" is not a file. "
                                                 "Please check the path.")

    def test_check_path_valid_should_return_error_when_is_file_and_path_is_link(self):
        path = '/path/a.so'
        is_file = True
        with mock.patch('os.path.exists', return_value=True), \
             mock.patch('os.path.isfile', return_value=True), \
             mock.patch('os.path.islink', return_value=True):
            with self.assertRaises(ProfException) as context:
                _check_path_valid(path, is_file)
        self.assertEqual(context.exception.code, ProfException.PROF_INVALID_PATH_ERROR)
        self.assertEqual(str(context.exception), "The path \"/path/a.so\" is link. Please check the path.")

    def test_check_path_valid_should_return_error_when_not_is_file_and_path_is_not_directory(self):
        path = '/path/host'
        is_file = False
        with mock.patch('os.path.exists', return_value=True), \
             mock.patch('os.path.isdir', return_value=False):
            with self.assertRaises(ProfException) as context:
                _check_path_valid(path, is_file)
        self.assertEqual(context.exception.code, ProfException.PROF_INVALID_PATH_ERROR)
        self.assertEqual(str(context.exception), "The path \"/path/host\" is not a directory. Please check the path.")

    def test_check_path_valid_should_return_error_when_is_not_file_and_path_is_link(self):
        path = '/path/host/host'
        is_file = False
        with mock.patch('os.path.exists', return_value=True), \
             mock.patch('os.path.isdir', return_value=True), \
             mock.patch('os.path.islink', return_value=True):
            with self.assertRaises(ProfException) as context:
                _check_path_valid(path, is_file)
        self.assertEqual(context.exception.code, ProfException.PROF_INVALID_PATH_ERROR)
        self.assertEqual(str(context.exception), "The path \"/path/host/host\" is link. Please check the path.")

    def test_check_path_valid_should_return_error_when_is_not_file_and_path_has_no_permission_to_read(self):
        path = '/path/host/host'
        is_file = False
        with mock.patch('os.path.exists', return_value=True), \
             mock.patch('os.path.isdir', return_value=True), \
             mock.patch('os.path.islink', return_value=False), \
             mock.patch('os.access', return_value=False):
            with self.assertRaises(ProfException) as context:
                _check_path_valid(path, is_file)
        self.assertEqual(context.exception.code, ProfException.PROF_INVALID_PATH_ERROR)
        self.assertEqual(str(context.exception),
                         "The path \"/path/host/host\" does not have "
                         "permission to read. Please check that the path is readable.")

    def test_check_path_valid_should_return_error_when_file_and_path_has_no_permission_to_read(self):
        path = '/path/host/test.so'
        is_file = True
        with mock.patch('os.path.exists', return_value=True), \
             mock.patch('os.path.isfile', return_value=True), \
             mock.patch('os.path.islink', return_value=False), \
             mock.patch('os.access', return_value=False):
            with self.assertRaises(ProfException) as context:
                _check_path_valid(path, is_file)
        self.assertEqual(context.exception.code, ProfException.PROF_INVALID_PATH_ERROR)
        self.assertEqual(str(context.exception),
                         "The path \"/path/host/test.so\" does not have "
                         "permission to read. Please check that the path is readable.")

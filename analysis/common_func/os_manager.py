#!/usr/bin/python3
# coding=utf-8
"""
Function:
This file mainly involves the os function.
Copyright Information:
Huawei Technologies Co., Ltd. All Rights Reserved © 2020
"""

import os

from common_func.common import error
from common_func.constant import Constant
from common_func.msprof_exception import ProfException


def _check_path_valid(path: str, is_file: bool) -> None:
    if path == "":
        error(os.path.basename(__file__),
              "The path is empty. Please enter a valid path.")
        raise ProfException(ProfException.PROF_INVALID_PARAM_ERROR)
    if not os.path.exists(path):
        error(os.path.basename(__file__),
              'The path "%s" does not exist. Please check that the '
              'path exists.' % path)
        raise ProfException(ProfException.PROF_INVALID_PATH_ERROR)
    if is_file:
        if not os.path.isfile(path):
            error(os.path.basename(__file__),
                  'The path "%s" is not a file. Please check the path.' % path)
            raise ProfException(ProfException.PROF_INVALID_PATH_ERROR)
    else:
        if not os.path.isdir(path):
            error(os.path.basename(__file__), 'The path "%s" is not a directory. Please check '
                                              'the path.' % path)
            raise ProfException(ProfException.PROF_INVALID_PATH_ERROR)
    if not os.access(path, os.R_OK):
        error(os.path.basename(__file__),
              'The path "%s" does not have permission to read.'
              ' Please check that the path is readable.' % path)
        raise ProfException(ProfException.PROF_INVALID_PATH_ERROR)


def check_file_readable(path: str) -> None:
    """
    check path is file and readable
    """
    _check_path_valid(path, True)
    if os.path.getsize(path) > Constant.MAX_READ_FILE_BYTES:
        error(os.path.basename(__file__), 'The "%s" is too large. Please check the path.' % path)
        raise ProfException(ProfException.PROF_INVALID_PATH_ERROR)


def check_file_executable(path: str) -> None:
    """
    check path is file and executable
    """
    _check_path_valid(path, True)
    if not os.access(path, os.X_OK):
        error(os.path.basename(__file__),
              'The path "%s" does not have permission to execute.'
              ' Please check that the path is executable.' % path)
        raise ProfException(ProfException.PROF_INVALID_PATH_ERROR)


def check_file_writable(path: str) -> None:
    """
    check path is file and writable
    """
    if path and os.path.exists(path):
        _check_path_valid(path, True)
        if not os.access(path, os.W_OK):
            error(os.path.basename(__file__),
                  'The path "%s" does not have permission to write.'
                  ' Please check that the path is writeable.' % path)
            raise ProfException(ProfException.PROF_INVALID_PATH_ERROR)


def check_dir_readable(path: str) -> None:
    """
    check path is dir and readable
    """
    _check_path_valid(path, False)


def check_dir_writable(path: str) -> None:
    """
    check path is dir and writable
    """
    _check_path_valid(path, False)
    if not os.access(path, os.W_OK):
        error(os.path.basename(__file__),
              'The path "%s" does not have permission to write.'
              ' Please check that the path is writeable.' % path)
        raise ProfException(ProfException.PROF_INVALID_PATH_ERROR)

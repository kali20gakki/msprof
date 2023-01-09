#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

import os

from common_func.common import error
from common_func.constant import Constant
from common_func.msprof_exception import ProfException


def _check_path_valid(path: str, is_file: bool) -> None:
    if path == "":
        raise ProfException(ProfException.PROF_INVALID_PARAM_ERROR, "The path is empty. Please enter a valid path.")
    if not os.path.exists(path):
        raise ProfException(ProfException.PROF_INVALID_PATH_ERROR,
                            f"The path \"{path}\" does not exist. Please check that the path exists.")
    if is_file:
        if not os.path.isfile(path):
            raise ProfException(ProfException.PROF_INVALID_PATH_ERROR,
                                f"The path \"{path}\" is not a file. Please check the path.")
    else:
        if not os.path.isdir(path):
            raise ProfException(ProfException.PROF_INVALID_PATH_ERROR,
                                f"The path \"{path}\" is not a directory. Please check the path.")
    if not os.access(path, os.R_OK):
        raise ProfException(ProfException.PROF_INVALID_PATH_ERROR,
                            f"The path \"{path}\" does not have permission to read. "
                            f"Please check that the path is readable.")


def check_file_readable(path: str) -> None:
    """
    check path is file and readable
    """
    _check_path_valid(path, True)
    if os.path.getsize(path) > Constant.MAX_READ_FILE_BYTES:
        raise ProfException(ProfException.PROF_INVALID_PATH_ERROR,
                            f"The \"{path}\" is too large. Please check the path.")


def check_file_executable(path: str) -> None:
    """
    check path is file and executable
    """
    _check_path_valid(path, True)
    if not os.access(path, os.X_OK):
        raise ProfException(ProfException.PROF_INVALID_PATH_ERROR,
                            f"The path \"{path}\" does not have permission to execute. "
                            f"Please check that the path is executable.")


def check_file_writable(path: str) -> None:
    """
    check path is file and writable
    """
    if path and os.path.exists(path):
        _check_path_valid(path, True)
        if not os.access(path, os.W_OK):
            raise ProfException(ProfException.PROF_INVALID_PATH_ERROR,
                                f"The path \"{path}\" does not have permission to write. "
                                f"Please check that the path is writeable.")


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
        raise ProfException(ProfException.PROF_INVALID_PATH_ERROR,
                            f"The path \"{path}\" does not have permission to write. "
                            f"Please check that the path is writeable.")

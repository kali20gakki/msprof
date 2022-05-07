#!/usr/bin/python3
# coding=utf-8
"""
Function:
This file mainly involves the exception class.
Copyright Information:
Huawei Technologies Co., Ltd. All Rights Reserved © 2020
"""


class ProfException(Exception):
    """
    The class for Profiling Exception
    """

    # error code for user:success
    PROF_NONE_ERROR = 0
    # error code for user: error
    PROF_INVALID_PARAM_ERROR = 1
    PROF_INVALID_PATH_ERROR = 2
    PROF_INVALID_CONNECT_ERROR = 3
    PROF_INVALID_EXECUTE_CMD_ERROR = 4
    PROF_INVALID_CONFIG_ERROR = 5
    PROF_INVALID_DISK_SHORT_ERROR = 6
    PROF_INVALID_JSON_ERROR = 7
    PROF_INVALID_DATA_ERROR = 8
    PROF_INVALID_STEP_TRACE_ERROR = 9
    PROF_SYSTEM_EXIT = 10

    def __init__(self: any, code: int) -> None:
        super().__init__(code)
        self.code = code

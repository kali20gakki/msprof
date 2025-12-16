#!/usr/bin/python3
# -*- coding: utf-8 -*-
# -------------------------------------------------------------------------
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
# This file is part of the MindStudio project.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#    http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------

import time
from abc import ABCMeta
from ..constant import MsptiResult, Constant
from ..utils import print_error_msg
from ._mspti_c import (
    _start,
    _stop,
    _flush_all,
    _flush_period,
    _set_buffer_size
)


class BaseMonitor(metaclass=ABCMeta):

    def __init__(self):
        super().__init__()

    @classmethod
    def start_monitor(cls) -> MsptiResult:
        return MsptiResult(_start())

    @classmethod
    def stop_monitor(cls) -> MsptiResult:
        try:
            ret = MsptiResult(_stop())
            if ret == MsptiResult.MSPTI_SUCCESS:
                time.sleep(Constant.FLUSH_SLEEP_TIME)
                return MsptiResult(_flush_all())
            return ret
        except Exception as ex:
            print_error_msg(f"Call stop failed. Exception: {str(ex)}")
            return MsptiResult.MSPTI_ERROR_INNER

    @classmethod
    def flush_all(cls) -> MsptiResult:
        try:
            return MsptiResult(_flush_all())
        except Exception as ex:
            print_error_msg(f"Call flush_all failed. Exception: {str(ex)}")
            return MsptiResult.MSPTI_ERROR_INNER

    @classmethod
    def flush_period(cls, time_ms: int) -> MsptiResult:
        return MsptiResult(_flush_period(time_ms))

    @classmethod
    def set_buffer_size(cls, size: int) -> MsptiResult:
        if not isinstance(size, int) or size <= 0:
            print_error_msg(f"Invalid buffer size {size}")
            return MsptiResult.MSPTI_ERROR_INVALID_PARAMETER
        if size > Constant.MAX_BUFFER_SIZE:
            print_error_msg(f"Buffer size {size} exceed the upper limit: {Constant.MAX_BUFFER_SIZE}")
            return MsptiResult.MSPTI_ERROR_INVALID_PARAMETER
        return MsptiResult(_set_buffer_size(size))

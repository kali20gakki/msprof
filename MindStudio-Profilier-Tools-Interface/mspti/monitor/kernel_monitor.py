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

from typing import Callable
from ..activity_data import KernelData
from ..constant import MsptiResult
from ..utils import print_error_msg
from .base_monitor import BaseMonitor
from ._mspti_c import (
    _kernel_register_cb,
    _kernel_unregister_cb
)


class KernelMonitor(BaseMonitor):

    def __init__(self):
        super().__init__()
        self.user_cb = None

    def start(self, cb: Callable[[KernelData], None]) -> MsptiResult:
        if not callable(cb):
            print_error_msg("Kernel callback is invalid")
            return MsptiResult.MSPTI_ERROR_INVALID_PARAMETER
        ret = BaseMonitor.start_monitor()
        if ret == MsptiResult.MSPTI_SUCCESS:
            self.user_cb = cb
            return MsptiResult(_kernel_register_cb(self.callback))
        return ret

    def stop(self) -> MsptiResult:
        ret = BaseMonitor.stop_monitor()
        if ret == MsptiResult.MSPTI_SUCCESS:
            self.user_cb = None
            return MsptiResult(_kernel_unregister_cb())
        return ret

    def callback(self, origin_data: dict):
        try:
            if callable(self.user_cb):
                self.user_cb(KernelData(origin_data))
        except Exception as ex:
            print_error_msg(f"Call kernel callback failed. Exception: {str(ex)}")

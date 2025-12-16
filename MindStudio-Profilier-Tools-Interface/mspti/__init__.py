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

"""
Functions and data structures used to declare public of mspti
"""

__all__ = [
    "KernelMonitor", "KernelData",
    "MstxMonitor", "MarkerData", "RangeMarkerData",
    "HcclMonitor", "HcclData",
    "MsptiObjectId", "MsptiResult", "MsptiActivityKind", "MsptiActivityFlag", "MsptiActivitySourceKind"
]

from .monitor.kernel_monitor import KernelMonitor
from .monitor.mstx_monitor import MstxMonitor
from .monitor.hccl_monitor import HcclMonitor
from .constant import (
    MsptiResult, MsptiActivityKind, MsptiActivityFlag, MsptiActivitySourceKind
)
from .activity_data import (
    KernelData, MarkerData, RangeMarkerData, HcclData, MsptiObjectId
)

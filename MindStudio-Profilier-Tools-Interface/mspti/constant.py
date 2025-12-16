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

from enum import Enum


class Constant:
    # max activity buffer size (MB)
    MAX_BUFFER_SIZE = 256
    # monitor flush sleep time (s)
    FLUSH_SLEEP_TIME = 0.05


class MsptiResult(Enum):
    MSPTI_SUCCESS = 0
    MSPTI_ERROR_INVALID_PARAMETER = 1
    MSPTI_ERROR_MULTIPLE_SUBSCRIBERS_NOT_SUPPORTED = 2
    MSPTI_ERROR_MAX_LIMIT_REACHED = 3
    MSPTI_ERROR_DEVICE_OFFLINE = 4
    MSPTI_ERROR_INNER = 999
    MSPTI_ERROR_FORCE_INT = 0x7fffffff


class MsptiActivityKind(Enum):
    MSPTI_ACTIVITY_KIND_INVALID = 0
    MSPTI_ACTIVITY_KIND_MARKER = 1
    MSPTI_ACTIVITY_KIND_KERNEL = 2
    MSPTI_ACTIVITY_KIND_API = 3
    MSPTI_ACTIVITY_KIND_HCCL = 4
    MSPTI_ACTIVITY_KIND_COUNT = 5
    MSPTI_ACTIVITY_KIND_FORCE_INT = 0x7fffffff


class MsptiActivityFlag(Enum):
    MSPTI_ACTIVITY_FLAG_NONE = 0
    MSPTI_ACTIVITY_FLAG_MARKER_INSTANTANEOUS = 1
    MSPTI_ACTIVITY_FLAG_MARKER_START = 1 << 1
    MSPTI_ACTIVITY_FLAG_MARKER_END = 1 << 2
    MSPTI_ACTIVITY_FLAG_MARKER_INSTANTANEOUS_WITH_DEVICE = 1 << 3
    MSPTI_ACTIVITY_FLAG_MARKER_START_WITH_DEVICE = 1 << 4
    MSPTI_ACTIVITY_FLAG_MARKER_END_WITH_DEVICE = 1 << 5


class MsptiActivitySourceKind(Enum):
    MSPTI_ACTIVITY_SOURCE_KIND_HOST = 0
    MSPTI_ACTIVITY_SOURCE_KIND_DEVICE = 1

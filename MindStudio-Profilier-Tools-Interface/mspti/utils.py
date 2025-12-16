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

import datetime
import os


def print_info_msg(message: str):
    time_str = datetime.datetime.utcnow().strftime("%Y-%m-%d %H:%M:%S")
    print(f"{time_str} [INFO] [MSPTI] [{os.getpid()}]: {message}")


def print_warn_msg(message: str):
    time_str = datetime.datetime.utcnow().strftime("%Y-%m-%d %H:%M:%S")
    print(f"{time_str} [WARNING] [MSPTI] [{os.getpid()}]: {message}")


def print_error_msg(message: str):
    time_str = datetime.datetime.utcnow().strftime("%Y-%m-%d %H:%M:%S")
    print(f"{time_str} [ERROR] [MSPTI] [{os.getpid()}]: {message}")

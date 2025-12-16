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

import os
import sys
import importlib

so_path = os.path.join(os.path.dirname(__file__), "..", "lib64")
sys.path.append(os.path.realpath(so_path))

mspti_C_module = importlib.import_module("mspti_C")
_start = mspti_C_module.start
_stop = mspti_C_module.stop
_flush_all = mspti_C_module.flush_all
_flush_period = mspti_C_module.flush_period
_set_buffer_size = mspti_C_module.set_buffer_size

mspti_C_mstx_module = importlib.import_module("mspti_C.mstx")
_mstx_register_cb = mspti_C_mstx_module.registerCB
_mstx_unregister_cb = mspti_C_mstx_module.unregisterCB
_mstx_enable_domain = mspti_C_mstx_module.enableDomain
_mstx_disable_domain = mspti_C_mstx_module.disableDomain

mspti_C_kernel_module = importlib.import_module("mspti_C.kernel")
_kernel_register_cb = mspti_C_kernel_module.registerCB
_kernel_unregister_cb = mspti_C_kernel_module.unregisterCB

mspti_C_hccl_module = importlib.import_module("mspti_C.hccl")
_hccl_register_cb = mspti_C_hccl_module.registerCB
_hccl_unregister_cb = mspti_C_hccl_module.unregisterCB
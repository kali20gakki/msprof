#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

import unittest
from unittest import mock

from msinterface.msprof_c_interface import _dump_cann_trace
from msinterface.msprof_c_interface import _dump_device_data
from msinterface.msprof_c_interface import _export_unified_db
from msinterface.msprof_c_interface import _export_timeline
from msinterface.msprof_c_interface import _export_op_summary

NAMESPACE = 'msinterface.msprof_c_interface'


class TestMsprofCInterface(unittest.TestCase):
    def test_dump_cann_trace(self):
        with mock.patch('importlib.import_module'):
            _dump_cann_trace("")

    def test_dump_device_data(self):
        with mock.patch('importlib.import_module'):
            _dump_device_data("")

    def test_export_unified_db(self):
        with mock.patch('importlib.import_module'):
            _export_unified_db("")

    def test_export_timeline(self):
        with mock.patch('importlib.import_module'):
            _export_timeline("")

    def test_export_op_summary(self):
        with mock.patch('importlib.import_module'):
            _export_op_summary("")
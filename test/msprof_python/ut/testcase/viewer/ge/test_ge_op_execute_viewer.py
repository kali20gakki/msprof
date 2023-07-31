#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import json
import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msvp_constant import MsvpConstant
from viewer.ge.ge_op_execute_viewer import GeOpExecuteViewer

NAMESPACE = 'viewer.ge.ge_op_execute_viewer'


class TestGeOpExecuteViewer(unittest.TestCase):
    def test_get_summary_data(self):
        with mock.patch(NAMESPACE + '.GeOpExecuteViewModel.init', return_value=False):
            obj = GeOpExecuteViewer({}, {"result_dir": "test"})
            result = obj.get_summary_data()
        self.assertEqual(MsvpConstant.MSVP_EMPTY_DATA, result)
        with mock.patch(NAMESPACE + '.GeOpExecuteViewModel.init', return_value=True), \
                mock.patch(NAMESPACE + '.GeOpExecuteViewModel.get_summary_data', return_value=[]):
            obj = GeOpExecuteViewer({}, {"result_dir": "test"})
            result = obj.get_summary_data()
        self.assertEqual(MsvpConstant.MSVP_EMPTY_DATA, result)
        InfoConfReader()._host_freq = None
        InfoConfReader()._info_json = {'CPU': [{'Frequency': "1000"}]}
        with mock.patch(NAMESPACE + '.GeOpExecuteViewModel.init', return_value=True), \
                mock.patch(NAMESPACE + '.GeOpExecuteViewModel.get_summary_data',
                           return_value=[[1, 'name', 'Cast', 'tiling', 0, 10]]):
            obj = GeOpExecuteViewer({"headers": ['Thread ID', 'OP Name', 'OP Type', 'Event Type',
                                                 'Start Time', 'Duration']},
                                    {"project": "test"})
            result = obj.get_summary_data()
        self.assertEqual(3, len(result))
        self.assertEqual((['Thread ID', 'OP Name', 'OP Type', 'Event Type', 'Start Time', 'Duration'],
                          [[1, 'name', 'Cast', 'tiling', 0.0, 0.01]], 1), result)

    def test_get_timeline_data(self):
        with mock.patch(NAMESPACE + '.GeOpExecuteViewModel.init', return_value=False):
            obj = GeOpExecuteViewer({}, {"result_dir": "test"})
            result = obj.get_timeline_data()
        self.assertEqual(NumberConstant.ERROR, json.loads(result).get("status"))
        with mock.patch(NAMESPACE + '.GeOpExecuteViewModel.init', return_value=True), \
                mock.patch(NAMESPACE + '.GeOpExecuteViewModel.get_timeline_data', return_value=[]):
            InfoConfReader()._info_json = {'devices': '0', "pid": "1"}
            obj = GeOpExecuteViewer({}, {"project": "test"})
            result = obj.get_timeline_data()
        self.assertEqual(1, json.loads(result)[0].get("pid"))
        with mock.patch(NAMESPACE + '.GeOpExecuteViewModel.init', return_value=True), \
                mock.patch(NAMESPACE + '.GeOpExecuteViewModel.get_timeline_data',
                           return_value=[[1, 'name', 'Cast', 'tiling', 0.0, 0.01]]):
            InfoConfReader()._host_freq = None
            InfoConfReader()._info_json = {'devices': '0', "pid": "1", 'CPU': [{'Frequency': "1000"}]}
            obj = GeOpExecuteViewer({}, {"project": "test"})
            result = obj.get_timeline_data()
        self.assertEqual(4, len(json.loads(result)))

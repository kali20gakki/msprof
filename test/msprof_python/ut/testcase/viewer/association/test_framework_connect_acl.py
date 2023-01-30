#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2023. All rights reserved.

import unittest
from unittest import mock

from profiling_bean.db_dto.msproftx_dto import MsprofTxDto
from viewer.association.framework_connect_acl import FrameworkToAcl

NAMESPACE = "viewer.association.framework_connect_acl"


class TestFrameworkToAcl(unittest.TestCase):
    JSON_DATA = [
        {
            "name": "aclopCompileAndExecute",
            "pid": "2_998314",
            "tid": 998314,
            "ts": 0.009,
            "dur": 939355.02,
            "ph": "X"
        },
        {
            "name": "aclopCompileAndExecute",
            "pid": "2_998314",
            "tid": 998314,
            "ts": 0.016,
            "dur": 64558.81,
            "ph": "X"
        }
    ]
    torch_data1 = MsprofTxDto()
    torch_data1.pid = 1
    torch_data1.tid = 2
    torch_data1.start_time = 10
    torch_data1.message = 'add'
    torch_data2 = MsprofTxDto()
    torch_data2.pid = 1
    torch_data2.tid = 2
    torch_data2.start_time = 11
    torch_data2.message = 'test1'
    cann_data1 = MsprofTxDto()
    cann_data1.pid = 1
    cann_data1.tid = 2
    cann_data1.start_time = 11
    cann_data1.message = 'add'
    cann_data2 = MsprofTxDto()
    cann_data2.pid = 1
    cann_data2.tid = 2
    cann_data2.start_time = 15
    cann_data2.message = 'mul'
    SIDE_EFFECT_MSPROFTX_DATA1 = [[torch_data1, torch_data2], [cann_data1, cann_data2]]
    SIDE_EFFECT_MSPROFTX_DATA2 = [[torch_data1, torch_data2], [cann_data1, cann_data2]]

    def test_get_connect_start_point(self) -> None:
        with mock.patch(NAMESPACE + '.MsprofTxModel.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.MsprofTxModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + '.MsprofTxModel.get_msproftx_data_by_file_tag',
                           side_effect=self.SIDE_EFFECT_MSPROFTX_DATA1):
            check = FrameworkToAcl('test', self.JSON_DATA)
            check._load_data()
            self.assertEqual(check._get_connect_start_point(),
                             [{'name': 'connect', 'ph': 's', 'id': 11, 'pid': '0_1', 'tid': 2, 'ts': 10 / 1000}])

    def test_get_connect_end_point(self) -> None:
        with mock.patch(NAMESPACE + '.MsprofTxModel.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.MsprofTxModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + '.MsprofTxModel.get_msproftx_data_by_file_tag',
                           side_effect=self.SIDE_EFFECT_MSPROFTX_DATA2):
            check = FrameworkToAcl('test', self.JSON_DATA)
            check._load_data()
            self.assertEqual(check._get_connect_end_point(),
                             [{'name': 'connect', 'ph': 'f', 'id': 15, 'pid': "2_998314", 'tid': 998314, 'ts': 0.016,
                               'bp': 'e'}])

    def test_add_connect_line(self) -> None:
        with mock.patch(NAMESPACE + '.MsprofTxModel.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.MsprofTxModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + '.MsprofTxModel.get_msproftx_data_by_file_tag',
                           side_effect=self.SIDE_EFFECT_MSPROFTX_DATA2), \
                mock.patch(NAMESPACE + '.FrameworkToAcl._get_connect_start_point', return_value=[
                    {'name': 'connect', 'ph': 's', 'id': 11, 'pid': '0_1', 'tid': 2, 'ts': 10 / 1000}]), \
                mock.patch(NAMESPACE + '.FrameworkToAcl._get_connect_end_point', return_value=[
                    {'name': 'connect', 'ph': 'f', 'id': 15, 'pid': "2_998314", 'tid': 998314, 'ts': 0.016,
                     'bp': 'e'}]):
            check = FrameworkToAcl('test', self.JSON_DATA)
            self.assertEqual(check.add_connect_line(),
                             [{'name': 'connect', 'ph': 's', 'id': 11, 'pid': '0_1', 'tid': 2, 'ts': 10 / 1000},
                              {'name': 'connect', 'ph': 'f', 'id': 15, 'pid': "2_998314", 'tid': 998314, 'ts': 0.016,
                               'bp': 'e'}])

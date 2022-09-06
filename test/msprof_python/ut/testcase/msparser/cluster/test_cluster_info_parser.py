#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2022. All rights reserved.
"""
import os
import unittest
from unittest import mock
from unittest.mock import PropertyMock

import pytest

from common_func.msprof_exception import ProfException
from constant.constant import clear_dt_project
from msmodel.cluster_info.cluster_info_model import ClusterInfoViewModel
from msparser.cluster.cluster_info_parser import ClusterInfoParser, ClusterBasicInfo

NAMESPACE = 'msparser.cluster.cluster_info_parser'


class TestClusterInfoParser(unittest.TestCase):
    DIR_PATH = os.path.join(os.path.dirname(__file__), 'DT_ClusterInfoParser')
    device_cluster_basic_info = {"PROF1\\device_0": ClusterBasicInfo('1'),
                                 "PROF2\\device_1": ClusterBasicInfo('1')}
    side_effect_rank_id = [0, 1]

    def setUp(self) -> None:
        os.makedirs(os.path.join(self.DIR_PATH, 'PROF1', 'device_0'))
        os.makedirs(os.path.join(self.DIR_PATH, 'PROF2', 'device_1'))
        os.makedirs(os.path.join(self.DIR_PATH, 'sqlite'))

    def tearDown(self) -> None:
        clear_dt_project(self.DIR_PATH)

    def test_ms_run_should_create_db(self):
        with mock.patch(NAMESPACE + '.ClusterBasicInfo.job_info', new_callable=PropertyMock, return_value='NA'), \
                mock.patch(NAMESPACE + '.ClusterBasicInfo.device_id', new_callable=PropertyMock, return_value=0), \
                mock.patch(NAMESPACE + '.ClusterBasicInfo.collection_time', new_callable=PropertyMock,
                           return_value='2022-09-01 16:34'), \
                mock.patch(NAMESPACE + '.ClusterBasicInfo.rank_id', new_callable=PropertyMock,
                           side_effect=self.side_effect_rank_id):
            check = ClusterInfoParser(self.DIR_PATH, self.device_cluster_basic_info)
            check.ms_run()
            with ClusterInfoViewModel(self.DIR_PATH) as _model:
                self.assertEqual(len(_model.get_all_rank_id()), 2)

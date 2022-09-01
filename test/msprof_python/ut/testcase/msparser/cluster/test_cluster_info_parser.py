#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2022. All rights reserved.
"""
import os
import unittest

import pytest

from common_func.msprof_exception import ProfException
from constant.constant import clear_dt_project
from msparser.cluster.cluster_info_parser import ClusterInfoParser

NAMESPACE = 'msparser.cluster.cluster_info_parser'


class TestClusterInfoParser(unittest.TestCase):
    DIR_PATH = os.path.join(os.path.dirname(__file__), 'DT_ClusterInfoParser')

    def setUp(self) -> None:
        os.makedirs(os.path.join(self.DIR_PATH, 'PROF1', 'devie_0'))
        os.makedirs(os.path.join(self.DIR_PATH, 'sqlite'))

    def tearDown(self) -> None:
        clear_dt_project(self.DIR_PATH)

    def test_ms_run_should_raise_exception_when_without_same_rank_id_exist(self):
        check = ClusterInfoParser(self.DIR_PATH, [os.path.join(self.DIR_PATH, 'PROF1', 'devie_0')])
        check.ms_run()

    def test_ms_run_should_raise_exception_when_the_same_rank_id_exist(self):
        with pytest.raises(ProfException) as err:
            check = ClusterInfoParser(self.DIR_PATH, [os.path.join(self.DIR_PATH, 'PROF1', 'devie_0')])
            check.rank_id_set.add('N/A')
            check.ms_run()
        self.assertEqual(ProfException.PROF_CLUSTER_DIR_ERROR, err.value.code)

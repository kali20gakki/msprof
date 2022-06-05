#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import unittest
from unittest import mock

from common_func.constant import Constant
from mscalculate.hccl.cluster_link_calculate import ClusterLinkCalculator
from mscalculate.hccl.cluster_link_calculate import ClusterSingleLinkCalculator

NAMESPACE = 'mscalculate.hccl.cluster_link_calculate'
SINGLE_NAMESPACE = 'mscalculate.hccl.cluster_link_calculate'


class TestClusterLinkCalculator(unittest.TestCase):
    file_list = [""]

    def test_run(self):
        with mock.patch(NAMESPACE + '.ClusterSingleLinkCalculator.ms_run',
                        return_value={Constant.TYPE_SDMA: [[5, 1, 0, 39]]}):
            check = ClusterLinkCalculator(self.file_list)
            check.run()
            self.assertEqual(check.result_dict,
                             {Constant.TYPE_SDMA: ['slow rank link 1-0, bandwidth is 39% lower than average.']})


class TestClusterSingleLinkCalculator(unittest.TestCase):

    def test_ms_run(self):
        with mock.patch(SINGLE_NAMESPACE + '.HCCLModel.init', return_value=True), \
             mock.patch(SINGLE_NAMESPACE + '.HCCLModel.finalize'), \
             mock.patch(SINGLE_NAMESPACE + '.HCCLModel.check_table', return_value=True), \
             mock.patch(SINGLE_NAMESPACE + '.HCCLModel.get_hccl_data', return_value=[1]), \
             mock.patch(SINGLE_NAMESPACE + '.ClusterSingleLinkCalculator.get_all_link_dict'), \
             mock.patch(SINGLE_NAMESPACE + '.ClusterSingleLinkCalculator.get_slow_link_data'):
            check = ClusterSingleLinkCalculator("")
            res = check.ms_run()
            self.assertEqual(res, {})

        with mock.patch(SINGLE_NAMESPACE + '.HCCLModel.init', return_value=True), \
             mock.patch(SINGLE_NAMESPACE + '.HCCLModel.finalize'), \
             mock.patch(SINGLE_NAMESPACE + '.HCCLModel.check_table', return_value=False), \
             mock.patch(SINGLE_NAMESPACE + '.HCCLModel.get_hccl_data', return_value=[1]), \
             mock.patch(SINGLE_NAMESPACE + '.ClusterSingleLinkCalculator.get_all_link_dict'), \
             mock.patch(SINGLE_NAMESPACE + '.ClusterSingleLinkCalculator.get_slow_link_data'):
            check = ClusterSingleLinkCalculator("")
            res = check.ms_run()
            self.assertEqual(res, {})

        with mock.patch(SINGLE_NAMESPACE + '.HCCLModel.init', return_value=True), \
             mock.patch(SINGLE_NAMESPACE + '.HCCLModel.finalize'), \
             mock.patch(SINGLE_NAMESPACE + '.HCCLModel.check_table', return_value=True), \
             mock.patch(SINGLE_NAMESPACE + '.HCCLModel.get_hccl_data', return_value=[]), \
             mock.patch(SINGLE_NAMESPACE + '.ClusterSingleLinkCalculator.get_all_link_dict'), \
             mock.patch(SINGLE_NAMESPACE + '.ClusterSingleLinkCalculator.get_slow_link_data'):
            check = ClusterSingleLinkCalculator("")
            res = check.ms_run()
            self.assertEqual(res, {})

    def test_get_slow_link_data(self):
        with mock.patch(SINGLE_NAMESPACE + '.ClusterSingleLinkCalculator.get_slow_link_by_type'):
            check = ClusterSingleLinkCalculator("")
            check.get_slow_link_data()

    def test_get_slow_link_by_type(self):
        check = ClusterSingleLinkCalculator("")
        check.link_dict = {Constant.TYPE_SDMA: [[5, 1, 0, 39], [7, 1, 0, 39], [9, 1, 0, 39]]}
        check.get_slow_link_by_type(Constant.TYPE_SDMA)

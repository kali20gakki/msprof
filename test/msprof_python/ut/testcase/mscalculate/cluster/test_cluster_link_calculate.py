#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import unittest
from unittest import mock

from mscalculate.cluster.cluster_link_calculate import ClusterLinkCalculator
from mscalculate.cluster.cluster_link_calculate import ClusterSingleLinkCalculator
from profiling_bean.db_dto.hccl_dto import HcclDto

NAMESPACE = 'mscalculate.cluster.cluster_link_calculate'


class TestClusterLinkCalculate(unittest.TestCase):

    def test_run(self):
        with mock.patch(NAMESPACE + '.ClusterSingleLinkCalculator.ms_run',
                        return_value={"SDMA": [['30', '0', '1', 30]]}):
            ret = ClusterLinkCalculator(["test"]).run()
            self.assertEqual(1, len(ret))


def construct_hccl_dto():
    data = [1, 2, 3, 4,
            "{'notify id': 4294967840, 'duration estimated': 0.8800048828125, 'stage': 4294967295, "
            "'step': 4294967385, 'bandwidth': 'NULL', 'stream id': 8, 'task id': 34, 'task type': 'Notify Record', "
            "'src rank': 2, 'dst rank': 1, 'transport type': 'SDMA', 'size': None, 'tag': 'all2allvc_1_5'}"]
    col = ["hccl_name", "plane_id", "timestamp", "duration", "args"]
    hccl_dto = HcclDto()
    for index, i in enumerate(data):
        if hasattr(hccl_dto, col[index]):
            setattr(hccl_dto, col[index], i)
    return hccl_dto


class TestClusterSingleLinkCalculator(unittest.TestCase):
    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.ClusterSingleLinkCalculator.calculate'):
            ret = ClusterSingleLinkCalculator("test").ms_run()
            self.assertEqual(0, len(ret))

    def test_calculate(self):
        with mock.patch(NAMESPACE + '.HCCLModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + '.HCCLModel.get_hccl_data',
                           return_value=[construct_hccl_dto(), construct_hccl_dto()]):
            ClusterSingleLinkCalculator("test").calculate()

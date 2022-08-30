#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import unittest
from unittest import mock

from common_func.db_name_constant import DBNameConstant
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


class TestClusterSingleLinkCalculator(unittest.TestCase):
    def setup_class(self):
        self.data = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, "RDMA", 15]
        col = ["hccl_name", "plane_id", "timestamp", "duration", "bandwidth",
               "stage", "step", "stream_id", "task_id", "task_type",
               "src_rank", "dst_rank", "notify_id", "transport_type", "size"]
        self.create_sql = "create table IF NOT EXISTS {0} " \
                          "(name TEXT, " \
                          "plane_id INTEGER, " \
                          "timestamp REAL, " \
                          "duration REAL, " \
                          "notify_id INTEGER, " \
                          "stage INTEGER, " \
                          "step INTEGER," \
                          "bandwidth REAL, " \
                          "stream_id INTEGER," \
                          "task_id INTEGER," \
                          "task_type TEXT," \
                          "src_rank INTEGER," \
                          "dst_rank INTEGER," \
                          "transport_type TEXT," \
                          "size TEXT)".format(DBNameConstant.TABLE_HCCL_ALL_REDUCE)
        self.hccl_dto = HcclDto()
        for index, i in enumerate(self.data):
            if hasattr(self.hccl_dto, col[index]):
                setattr(self.hccl_dto, col[index], i)

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.ClusterSingleLinkCalculator.calculate'):
            ret = ClusterSingleLinkCalculator("test").ms_run()
            self.assertEqual(0, len(ret))

    def test_calculate(self):
        with mock.patch(NAMESPACE + '.HCCLModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + '.HCCLModel.get_hccl_data',
                           return_value=[self.hccl_dto, self.hccl_dto]):
            ClusterSingleLinkCalculator("test").calculate()

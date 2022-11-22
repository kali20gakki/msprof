#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

import unittest

from profiling_bean.db_dto.hccl_op_dto import HcclOpDto

NAMESPACE = 'profiling_bean.db_dto.hccl_op_dto'


class TestHcclOpDto(unittest.TestCase):

    def test_construct(self):
        data = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18]
        col = ["_op_name", "_iteration", "_hccl_name", "_first_timestamp", "_plane_id", "_timestamp",
               "_duration", "_bandwidth", "_stage", "_step", "_stream_id", "_task_id", "_task_type",
               "_src_rank", "_dst_rank", "_notify_id", "_transport_type", "_size"]
        test = HcclOpDto()
        for index, i in enumerate(data):
            if hasattr(test, col[index]):
                setattr(test, col[index], i)
        test.op_name = 1
        test.iteration = 2
        test.hccl_name = 3
        test.first_timestamp = 4
        test.plane_id = 5
        test.timestamp = 6
        test.duration = 7
        test.bandwidth = 8
        test.stage = 9
        test.step = 10
        test.stream_id = 11
        test.task_id = 12
        test.task_type = 13
        test.src_rank = 14
        test.dst_rank = 15
        test.notify_id = 16
        test.transport_type = 17
        test.size = 18
        self.assertEqual(test.op_name, 1)
        self.assertEqual(test.iteration, 2)
        self.assertEqual(test.hccl_name, 3)
        self.assertEqual(test.first_timestamp, 4)
        self.assertEqual(test.plane_id, 5)
        self.assertEqual(test.timestamp, 6)
        self.assertEqual(test.duration, 7)
        self.assertEqual(test.bandwidth, 8)
        self.assertEqual(test.stage, 9)
        self.assertEqual(test.step, 10)
        self.assertEqual(test.stream_id, 11)
        self.assertEqual(test.task_id, 12)
        self.assertEqual(test.task_type, 13)
        self.assertEqual(test.src_rank, 14)
        self.assertEqual(test.dst_rank, 15)
        self.assertEqual(test.notify_id, 16)
        self.assertEqual(test.transport_type, 17)
        self.assertEqual(test.size, 18)


if __name__ == '__main__':
    unittest.main()

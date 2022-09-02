#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2022. All rights reserved.
"""
import unittest

from profiling_bean.db_dto.collective_communication_dto import CollectiveCommunicationDto


class TestCollectiveCommunicationDto(unittest.TestCase):
    def test_collective_communication_dto(self):
        check = CollectiveCommunicationDto()
        check.rank_id = 1
        check.compute_time = 123
        check.communication_time = 124
        check.stage_time = 125
        self.assertEqual([1, 123, 124, 125],
                         [check.rank_id, check.compute_time, check.communication_time, check.stage_time])

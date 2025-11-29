#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.

import unittest

from profiling_bean.stars.pmu_bean_v6 import PmuBeanV6


class TestPmuBeanV6(unittest.TestCase):
    """
     唯一ID合并stream id和task id, PMU仅上报task id
    """

    def test_instance_of_v6_bean_and_is_aic_data(self):
        args = (
            42, 27603, 1, 0, 642379, 6, 3, 0, 128, 15, 16, 17, 18, 0, 10, 11, 12, 13,
            14, 15, 16, 17, 18, 19, 2201803260082, 2201803277549
        )
        pmu_bean = PmuBeanV6(args)
        ret = [
            1, 1, 642379, 4294967295, 0, 15, 17, 18,
            (10, 11, 12, 13, 14, 15, 16, 17, 18, 19),
            2201803260082, 2201803277549
        ]
        bean_property = [
            pmu_bean.stream_id,
            pmu_bean.task_id,
            pmu_bean.total_cycle,
            pmu_bean.subtask_id,
            pmu_bean.core_type,
            pmu_bean.core_id,
            pmu_bean.sub_block_id,
            pmu_bean.block_id,
            pmu_bean.pmu_list,
            pmu_bean.start_time,
            pmu_bean.end_time
        ]
        self.assertEqual(ret, bean_property)
        self.assertTrue(pmu_bean.is_aic_data())

    def test_instance_of_v6_bean_and_is_not_mix_data(self):
        args = (
            42, 27603, 1, 0, 642379, 6, 0, 0, 128, 15, 16, 17, 18, 0, 10, 11, 12, 13,
            14, 15, 16, 17, 18, 19, 2201803260082, 2201803277549
        )
        pmu_bean = PmuBeanV6(args)
        ret = [
            1, 1, 642379, 4294967295, 0, 15, 17, 18,
            (10, 11, 12, 13, 14, 15, 16, 17, 18, 19),
            2201803260082, 2201803277549
        ]
        bean_property = [
            pmu_bean.stream_id,
            pmu_bean.task_id,
            pmu_bean.total_cycle,
            pmu_bean.subtask_id,
            pmu_bean.core_type,
            pmu_bean.core_id,
            pmu_bean.sub_block_id,
            pmu_bean.block_id,
            pmu_bean.pmu_list,
            pmu_bean.start_time,
            pmu_bean.end_time
        ]
        self.assertEqual(ret, bean_property)
        self.assertFalse(pmu_bean.is_mix_data())

    def test_instance_of_v6_bean_and_is_aiv_data(self):
        args = (
            42, 27603, 1, 0, 642379, 6, 0, 0, 129, 15, 16, 17, 18, 0, 10, 11, 12, 13,
            14, 15, 16, 17, 18, 19, 2201803260082, 2201803277549
        )
        pmu_bean = PmuBeanV6(args)
        ret = [
            1, 1, 642379, 4294967295, 1, 15, 17, 18,
            (10, 11, 12, 13, 14, 15, 16, 17, 18, 19),
            2201803260082, 2201803277549
        ]
        bean_property = [
            pmu_bean.stream_id,
            pmu_bean.task_id,
            pmu_bean.total_cycle,
            pmu_bean.subtask_id,
            pmu_bean.core_type,
            pmu_bean.core_id,
            pmu_bean.sub_block_id,
            pmu_bean.block_id,
            pmu_bean.pmu_list,
            pmu_bean.start_time,
            pmu_bean.end_time
        ]
        self.assertEqual(ret, bean_property)
        self.assertFalse(pmu_bean.is_aic_data())

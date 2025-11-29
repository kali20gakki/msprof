#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.

import unittest

from profiling_bean.stars.acsq_task_v6_bean import AcsqTaskV6


class TestAcsqTaskV6(unittest.TestCase):

    def test_instance_of_v6_task_bean(self):
        args = [1024, 27603, 3, 0, 2201803260082, 11, 65]
        acsq_task_v6 = AcsqTaskV6(args)
        ret = ['000000', 1, 3, 3, 1]
        bean_property = [
            acsq_task_v6.func_type,
            acsq_task_v6.task_type,
            acsq_task_v6.stream_id,
            acsq_task_v6.task_id,
            acsq_task_v6.acc_id
        ]
        self.assertEqual(ret, bean_property)

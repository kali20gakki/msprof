#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

import unittest
from tuning.cluster.cluster_calculator_factory import SlowRankCalculatorFactory
from tuning.cluster.cluster_calculator_factory import SlowLinkCalculatorFactory

NAMESPACE = 'tuning.cluster.cluster_calculator_factory'


class TestSlowRankCalculatorFactory(unittest.TestCase):
    def test_generate_calculator(self):
        op_info = {'hccl_name': {}}
        SlowRankCalculatorFactory(op_info)


class TestSlowLinkCalculatorFactory(unittest.TestCase):
    def test_generate_calculator(self):
        op_info = {'hccl_name': {1: {}}}
        SlowLinkCalculatorFactory(op_info)
#!/usr/bin/env python
# coding=utf-8
"""
function: test l2_cache_metric
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""

import unittest
from unittest import mock

from mscalculate.l2_cache.l2_cache_metric import HitRateMetric, VictimRateMetric
from common_func.info_conf_reader import InfoConfReader

from constant.constant import CONFIG

NAMESPACE = 'mscalculate.l2_cache.l2_cache_metric'

class TestHitRateMetric(unittest.TestCase):
    def test_calculate_hit_rate(self: any):
        hit_rate_metric = HitRateMetric(10, 35)
        self.assertEqual(hit_rate_metric._calculate_hit_rate(), 0.285714)
        hit_rate_metric = HitRateMetric(10, 0)
        self.assertEqual(hit_rate_metric._calculate_hit_rate(), 0)

class TestVictimRateMetric(unittest.TestCase):
    def test_calculate_victim_rate(self: any):
        victim_rate_metric = HitRateMetric(10, 35)
        self.assertEqual(victim_rate_metric._calculate_hit_rate(), 0.285714)
        victim_rate_metric = HitRateMetric(10, 0)
        self.assertEqual(victim_rate_metric._calculate_hit_rate(), 0)
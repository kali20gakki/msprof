#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

import unittest
from unittest import mock
from msparser.cluster.critical_path_parser import CriticalPathParser

NAMESPACE = 'msparser.cluster.critical_path_parser'


class TestCriticalPathParser(unittest.TestCase):
    def test_run(self):
        test_events = [
            {"op_name": "MatMul", "task_type": "AI_CORE", "tid": 2, "ts": 1, "es": 3, "dur": 2.},
            {"op_name": "allReduce", "task_type": "HCCL", "tid": 8, "ts": 4, "es": 5, "dur": 1}
        ]
        with mock.patch(NAMESPACE + '.CriticalPathParser.parse_ge_ops'), \
                mock.patch(NAMESPACE + '.CriticalPathParser.parse_hccl_ops'):
            critical_path_parser = CriticalPathParser([], {})
            critical_path_parser.events = test_events
            top_op = critical_path_parser.run()
            self.assertEqual(top_op, ('allReduce',))

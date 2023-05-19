#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
"""
import collections

import unittest
from unittest import mock

from common_func.constant import Constant
from mscalculate.cann.cann_analysis_chain import CANNAnalysisChain
from mscalculate.cann.cann_analysis_gear import RootGear
from mscalculate.cann.event import Event
from mscalculate.cann.event_queue import EventQueue
from mscalculate.cann.tree import TreeNode

NAMESPACE = 'mscalculate.cann.cann_analysis_chain'


class TestCANNAnalysisChain(unittest.TestCase):
    event_col = collections.namedtuple('Event', 'level, thread, start, end, type_')

    def test_start(self):
        chain = CANNAnalysisChain(1, EventQueue(), {Constant.ROOT_LEVEL: RootGear("")})
        with mock.patch(NAMESPACE + '.CANNAnalysisChain.build_tree',
                        return_value=TreeNode(Event(Constant.ROOT_LEVEL, 1, 0, 300, 'root'))):
            chain.start()

    def test_build_tree(self):
        def add_event(q, event_col):
            event = Event(event_col.level, event_col.thread, event_col.start, event_col.end, event_col.type_)
            q.add(event)

        def build_event_q():
            q = EventQueue()
            add_event(q, self.event_col(Constant.MODEL_LEVEL, 1, 100, 200, 'ModelLoad'))
            add_event(q, self.event_col(Constant.NODE_LEVEL, 1, 110, 130, 'node1'))
            add_event(q, self.event_col(Constant.NODE_LEVEL, 1, 150, 170, 'node2'))
            add_event(q, self.event_col(Constant.NODE_LEVEL, 1, 130, 130, 'node_basic_info1'))
            add_event(q, self.event_col(Constant.NODE_LEVEL, 1, 170, 170, 'node_basic_info2'))
            add_event(q, self.event_col(Constant.TASK_LEVEL, 1, 116, 120, 'task1'))
            add_event(q, self.event_col(Constant.TASK_LEVEL, 1, 152, 160, 'task2'))
            add_event(q, self.event_col(Constant.TASK_LEVEL, 1, 118, 118, 'task_track1'))
            add_event(q, self.event_col(Constant.TASK_LEVEL, 1, 155, 155, 'task_track2'))
            add_event(q, self.event_col(Constant.TASK_LEVEL, 1, 180, 190, 'task3'))
            add_event(q, self.event_col(Constant.TASK_LEVEL, 1, 210, 222, 'task4'))
            return q

        event_q = build_event_q()
        chain = CANNAnalysisChain(1, event_q, {})
        root_node = TreeNode(Event(Constant.ROOT_LEVEL, 1, 0, 300, 'root'))
        tree = chain.build_tree(root_node)
        # check level1
        self.assertEqual(len(tree.children), 2)
        self.assertEqual(tree.children[1].event.struct_type, "task4")
        # check level2
        self.assertEqual(len(tree.children[0].children), 3)
        self.assertEqual(tree.children[0].children[0].event.struct_type, "node1")
        self.assertEqual(tree.children[0].children[1].event.struct_type, "node2")
        self.assertEqual(tree.children[0].children[2].event.struct_type, "task3")
        self.assertEqual(len(tree.children[0].children[0].event.additional_record), 1)
        self.assertEqual(len(tree.children[0].children[1].event.additional_record), 1)
        # check level3
        self.assertEqual(len(tree.children[0].children[0].children), 1)
        self.assertEqual(len(tree.children[0].children[1].children), 1)
        self.assertEqual(tree.children[0].children[0].children[0].event.struct_type, "task1")
        self.assertEqual(tree.children[0].children[1].children[0].event.struct_type, "task2")
        self.assertEqual(len(tree.children[0].children[0].children[0].event.additional_record), 1)
        self.assertEqual(len(tree.children[0].children[1].children[0].event.additional_record), 1)


if __name__ == '__main__':
    unittest.main()

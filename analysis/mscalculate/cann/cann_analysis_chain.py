#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

from common_func.constant import Constant
from mscalculate.cann.cann_database import AdditionalRecordDatabase
from mscalculate.cann.cann_database import ApiDataDatabase
from mscalculate.cann.event import Event
from mscalculate.cann.event_queue import EventQueue
from mscalculate.cann.tree import TreeNode
from profiling_bean.db_dto.api_data_dto import ApiDataDto


class CANNAnalysisChain:
    """
    The analysis chain for CANN software stack. Each instance processes the data of a thread in the CANN.
    """

    def __init__(self, thread_id: int, event_q: EventQueue, gears: dict):
        """
        thread_id：
        event_q: priority queue with thread id
        gears: gear set, one gear process one cann level
        """
        self.thread_id = thread_id
        self.event_q = event_q
        self.gears = gears
        self.last_event_record = {
            Constant.MODEL_LEVEL: Event.invalid_event(),
            Constant.NODE_LEVEL: Event.invalid_event(),
            Constant.TASK_LEVEL: Event.invalid_event(),
            Constant.HCCL_LEVEL: Event.invalid_event()
        }
        self.now_stack = {
            Constant.MODEL_LEVEL: Event.invalid_event(),
            Constant.NODE_LEVEL: Event.invalid_event(),
            Constant.TASK_LEVEL: Event.invalid_event(),
            Constant.HCCL_LEVEL: Event.invalid_event()
        }

    def start(self):
        root_dto = ApiDataDto.invalid_dto(Constant.ROOT_LEVEL, self.thread_id, 0,
                                          ApiDataDatabase().get_max_bound() + 1, "root")
        root_event = ApiDataDatabase().put(root_dto)
        # Associate the upper-level and lower-level relationships of events
        # based on timestamp points to build a callstack tree.
        root = TreeNode(root_event)
        event_tree = self.build_tree(root)
        # Perform inter-layer association.
        self.run(event_tree)

    def run(self, node: TreeNode):
        self.gears.get(node.event.cann_level).run(node.event, self.now_stack)
        self.now_stack[node.event.cann_level] = node.event
        for child in node.children:
            self.run(child)
        self.now_stack[node.event.cann_level] = Event.invalid_event()

    def build_tree(self, parent: TreeNode) -> TreeNode:
        while True:
            # All processing is complete
            if self.event_q.empty(self.thread_id):
                return parent
            event = self.event_q.top(self.thread_id)

            # the current level has been processed.
            if not event.is_additional() and event.cann_level <= parent.event.cann_level:
                return parent
            # Some data in parent level is lost or not be uploaded.
            if event.bound > parent.event.bound:
                return parent

            event = self.event_q.pop(self.thread_id)
            if event.is_additional():
                last_event = self.last_event_record.get(event.cann_level)
                if last_event and last_event.timestamp <= event.timestamp <= last_event.bound:
                    # This event is a supplementary information, not an independent task unit.
                    # Events are sorted by timestamp. Therefore, this information is recorded in the last
                    # processed event.
                    last_event.add_additional_record(AdditionalRecordDatabase().get(event))
                    continue
                else:
                    # This scenario indicates that the event corresponding to the additional information is not
                    # reported. Hanging on a tree as an empty node
                    # 1. Start and end tasks of runtime
                    # 2 ....
                    empty_dto = ApiDataDto().invalid_dto(event.cann_level, event.thread_id)
                    empty_event: Event = ApiDataDatabase().put(empty_dto)
                    empty_event.add_additional_record(AdditionalRecordDatabase().get(event))
                    parent.add_child(TreeNode(empty_event))
                    continue

            self.last_event_record[event.cann_level] = event
            child_node = TreeNode(event)
            child_tree = self.build_tree(child_node)
            parent.add_child(child_tree)

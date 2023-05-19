#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
from queue import PriorityQueue

import typing

from mscalculate.cann.event import Event


class EventQueue:
    """
    This class is used for unified scheduling of events at different levels.
    1. Provides access to the event with the highest priority in the specified thread_id.
    """

    def __init__(self):
        self.queues: typing.Dict = dict()

    def add(self, event: Event):
        thread_id = event.thread_id
        if not self.queues.get(thread_id):
            self.queues[thread_id] = PriorityQueue()
        self.queues.get(thread_id).put(event)

    def pop(self, thread_id):
        event = self.queues.get(thread_id).get()
        return event

    def top(self, thread_id) -> Event:
        q = self.queues.get(thread_id)
        if not q or q.empty():
            return Event.invalid_event()
        return q.queue[0]

    def empty(self, thread_id):
        q = self.queues.get(thread_id)
        return not q or q.empty()

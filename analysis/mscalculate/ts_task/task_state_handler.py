#!/usr/bin/python3
# coding=utf-8
"""
function: this script used to operate ts track
Copyright Huawei Technologies Co., Ltd. 2021. All rights reserved.
"""


class TaskTimeline:
    def __init__(self: any, stream_id: int, task_id: int) -> None:
        self.stream_id = stream_id
        self.task_id = task_id
        self.start = None
        self.end = None


class TaskStateHandler:
    RECEIVE_TAG = 0
    START_TAG = 1
    END_TAG = 2

    def __init__(self: any, stream_id: int, task_id: int) -> None:
        self.stream_id = stream_id
        self.task_id = task_id
        self.new_task = None
        self.task_timeline_list = []

    def process_record(self: any, timestamp: int, task_state: int) -> None:
        if task_state == self.START_TAG:
            self.new_task = TaskTimeline(self.stream_id, self.task_id)
            self.new_task.start = timestamp

        if task_state == self.END_TAG and self.new_task:
            self.new_task.end = timestamp
            self.task_timeline_list.append(self.new_task)

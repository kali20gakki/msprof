#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
import unittest
from unittest import mock

from profiling_bean.db_dto.msproftx_dto import MsprofTxDto
from viewer.task_queue_viewer import TaskQueueViewer

NAMESPACE = "viewer.task_queue_viewer"


class TestTaskQueueViewer(unittest.TestCase):
    tid_meta1 = MsprofTxDto()
    tid_meta1.category = 0
    tid_meta1.tid = 11
    tid_meta2 = MsprofTxDto()
    tid_meta2.category = 1
    tid_meta2.tid = 12
    tid_meta = [tid_meta1, tid_meta2]
    task_queue1 = MsprofTxDto()
    task_queue1.pid = 1
    task_queue1.tid = 11
    task_queue1.category = 0
    task_queue1.start_time = 1
    task_queue1.message = 'Add'
    task_queue2 = MsprofTxDto()
    task_queue2.pid = 1
    task_queue2.tid = 11
    task_queue2.category = 0
    task_queue2.start_time = 3
    task_queue2.message = 'Add'
    task_queue3 = MsprofTxDto()
    task_queue3.pid = 1
    task_queue3.tid = 12
    task_queue3.category = 1
    task_queue3.start_time = 5
    task_queue3.message = 'Add'
    task_queue4 = MsprofTxDto()
    task_queue4.pid = 1
    task_queue4.tid = 12
    task_queue4.category = 1
    task_queue4.start_time = 10
    task_queue4.message = 'Add'
    task_queue = [task_queue1, task_queue2, task_queue3, task_queue4]

    result = '[{"name": "Add", "pid": 1, "tid": 11, "ts": 0.001, "ph": "X", "args": {}, "dur": 0.002}, {"name": ' \
             '"Add", "pid": 1, "tid": 12, "ts": 0.005, "ph": "X", "args": {}, "dur": 0.005}, {"name": "process_name",' \
             ' "pid": 1, "tid": 0, "args": {"name": "PTA"}, "ph": "M"}, {"name": "thread_name", "pid": 1, "tid": 11,' \
             ' "args": {"name": "Thread 11 (Enqueue)"}, "ph": "M"}, {"name": "thread_sort_index", "pid": 1, "tid": 11,'\
             ' "args": {"sort_index": 0}, "ph": "M"}, {"name": "thread_name", "pid": 1, "tid": 12, "args": {"name":' \
             ' "Thread 12 (Dequeue)"}, "ph": "M"}, {"name": "thread_sort_index", "pid": 1, "tid": 12, "args": ' \
             '{"sort_index": 1}, "ph": "M"}]'

    def test_get_task_queue_data(self):
        with mock.patch(NAMESPACE + '.MsprofTxModel.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.MsprofTxModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + '.MsprofTxModel.get_task_queue_origin_data', return_value=self.task_queue), \
                mock.patch(NAMESPACE + '.MsprofTxModel.get_task_queue_tid_meta_data', return_value=self.tid_meta):
            check = TaskQueueViewer({"project": "test"})
            self.assertEqual(self.result, check.get_task_queue_data())

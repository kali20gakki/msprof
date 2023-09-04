#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2023. All rights reserved.

import unittest
from unittest import mock

from viewer.association.host_connect_device import HostToDevice

NAMESPACE = "viewer.association.host_connect_device"


class TestHostToDevice(unittest.TestCase):
    def test_add_connect_line_api(self):
        api_data = [
            {
                "name": "Node@launch", "pid": 228731802, "tid": 2287318, "ts": 134524588477.77002,
                "dur": 390.26, "ph": "X",
                "args": {
                    "Thread Id": 2287318, "Mode": "launch", "level": "node", "id": "0",
                    "item_id": "hcom_broadcast_", "connection_id": 19,
                }
            },
            {
                "name": "Node@launch", "pid": 228731802, "tid": 2290334, "ts": 134525368854.88, "dur": 12.6, "ph": "X",
                "args": {
                    "Thread Id": 2290334, "Mode": "launch", "level": "node", "id": "0",
                    "item_id": "ApplyAdamW", "connection_id": 14983
                },
            },
        ]
        expected_data = api_data + [
            {
                "name": "HostToDevice81604378623", "ph": "s", "cat": "HostToDevice", "id": "81604378623",
                "pid": 228731802, "tid": 2287318, "ts": 134524588477.77002
            },
            {
                "name": "HostToDevice64355789963263", "ph": "s", "cat": "HostToDevice", "id": "64355789963263",
                "pid": 228731802, "tid": 2290334, "ts": 134525368854.88
            },
        ]
        with mock.patch(NAMESPACE + '.HostToDevice.get_node_tasks', return_value={(4, 33573, 23), (4, 33574, 23)}), \
                mock.patch(NAMESPACE + ".HostToDevice.get_connection_id_to_context_ids_mapping",
                           return_value={14983: [4294967295]}):
            connection = HostToDevice("")
            connection.add_connect_line(api_data, "api")
            self.assertEqual(api_data, expected_data)

    def test_add_connect_line_task_time(self):
        task_time_data = [
            {
                "name": "SDMA", "pid": 3, "tid": 11, "ts": 134524588800.38, "dur": 0.74, "ph": "X",
                "args": {
                    "Task Type": "SDMA", "Stream Id": 11, "Task Id": 4043,
                    "Batch Id": 0, "Subtask Id": 0, "connection_id": 19,
                }
            },
            {
                "name": "ApplyAdamW", "pid": 3, "tid": 4, "ts": 134525368863, "dur": 31.92, "ph": "X",
                "args": {
                    "Task Type": "AI_CORE", "Stream Id": 4, "Task Id": 33573,
                    "Batch Id": 23, "Subtask Id": 4294967295, "connection_id": 14983
                },
            },
        ]
        expected_data = task_time_data + [
            {
                "name": "HostToDevice64355789963263", "ph": "f", "id": "64355789963263", "ts": 134525368863,
                "cat": "HostToDevice", "pid": 3, "tid": 4, "bp": "e"
            },
        ]
        with mock.patch(NAMESPACE + '.HostToDevice.get_node_tasks', return_value={(4, 33573, 23), (4, 33574, 23)}), \
                mock.patch(NAMESPACE + ".HostToDevice.get_connection_id_to_context_ids_mapping",
                           return_value={14983: [4294967295]}):
            connection = HostToDevice("")
            connection.add_connect_line(task_time_data, "task_time")
            self.assertEqual(task_time_data, expected_data)

    def test_add_connect_line_hccl(self):
        hccl_data = [
            {
                "name": "hcom_broadcast__072_0", "pid": 228731811, "tid": 0, "ts": 134524588800.38, "dur": 0.74,
                "args": {"connection_id": 19}, "ph": "X",
            },
        ]
        expected_data = hccl_data + [
            {
                "name": "HostToDevice81604378623", "ph": "f", "id": "81604378623", "ts": 134524588800.38,
                "cat": "HostToDevice", "pid": 228731811, "tid": 0, "bp": "e"
            },
        ]
        with mock.patch(NAMESPACE + '.HostToDevice.get_node_tasks', return_value={(4, 33573, 23), (4, 33574, 23)}), \
                mock.patch(NAMESPACE + ".HostToDevice.get_connection_id_to_context_ids_mapping",
                           return_value={14983: [4294967295]}):
            connection = HostToDevice("")
            connection.add_connect_line(hccl_data, "hccl")
            self.assertEqual(hccl_data, expected_data)

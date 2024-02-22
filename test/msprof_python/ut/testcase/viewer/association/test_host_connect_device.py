#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2023. All rights reserved.

import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from viewer.association.host_connect_device import HostToDevice
from common_func.trace_view_header_constant import TraceViewHeaderConstant


NAMESPACE = "viewer.association.host_connect_device"


class TestHostToDevice(unittest.TestCase):
    def test_add_connect_line_api(self):
        api_data = [
            {
                TraceViewHeaderConstant.TRACE_HEADER_NAME: "Node@launch",
                TraceViewHeaderConstant.TRACE_HEADER_PID: 228731802,
                TraceViewHeaderConstant.TRACE_HEADER_TID: 2287318,
                TraceViewHeaderConstant.TRACE_HEADER_TS: 134524588477.77002,
                TraceViewHeaderConstant.TRACE_HEADER_DURATION: 390.26,
                TraceViewHeaderConstant.TRACE_HEADER_PH: "X",
                TraceViewHeaderConstant.TRACE_HEADER_ARGS: {
                    "Thread Id": 2287318, "Mode": "launch", "level": "node",
                    TraceViewHeaderConstant.TRACE_HEADER_ID: "0",
                    "item_id": "hcom_broadcast_", "connection_id": 19,
                }
            },
            {
                TraceViewHeaderConstant.TRACE_HEADER_NAME: "Node@launch",
                TraceViewHeaderConstant.TRACE_HEADER_PID: 228731802,
                TraceViewHeaderConstant.TRACE_HEADER_TID: 2290334,
                TraceViewHeaderConstant.TRACE_HEADER_TS: 134525368854.88,
                TraceViewHeaderConstant.TRACE_HEADER_DURATION: 12.6,
                TraceViewHeaderConstant.TRACE_HEADER_PH: "X",
                TraceViewHeaderConstant.TRACE_HEADER_ARGS: {
                    "Thread Id": 2290334, "Mode": "launch", "level": "node",
                    TraceViewHeaderConstant.TRACE_HEADER_ID: "0",
                    "item_id": "ApplyAdamW", "connection_id": 14983
                },
            },
        ]
        expected_data = api_data + [
            {
                TraceViewHeaderConstant.TRACE_HEADER_NAME: "HostToDevice64355789963263",
                TraceViewHeaderConstant.TRACE_HEADER_PH: "s",
                TraceViewHeaderConstant.TRACE_HEADER_CAT: "HostToDevice",
                TraceViewHeaderConstant.TRACE_HEADER_ID: "64355789963263",
                TraceViewHeaderConstant.TRACE_HEADER_PID: 228731802,
                TraceViewHeaderConstant.TRACE_HEADER_TID: 2290334,
                TraceViewHeaderConstant.TRACE_HEADER_TS: 134525368854.88
            }
        ]
        with mock.patch(NAMESPACE + '.HostToDevice.get_node_tasks',
                        return_value={(4, 33573, 23): (2287318, 4294967295), (4, 33574, 23): (2287318, 4294967295)}), \
                mock.patch(NAMESPACE + ".HostToDevice.get_hccl_op_connection_ids", return_value={14983}), \
                mock.patch(NAMESPACE + ".HostToDevice.get_connection_id_to_context_ids_mapping",
                           return_value={14983: [4294967295]}):
            connection = HostToDevice("")
            connection.add_connect_line(api_data, "api")
            self.assertEqual(api_data, expected_data)

    def test_add_connect_line_task_time(self):
        task_time_data = [
            {
                TraceViewHeaderConstant.TRACE_HEADER_NAME: "SDMA",
                TraceViewHeaderConstant.TRACE_HEADER_PID: 3,
                TraceViewHeaderConstant.TRACE_HEADER_TID: 11,
                TraceViewHeaderConstant.TRACE_HEADER_TS: 134524588800.38,
                TraceViewHeaderConstant.TRACE_HEADER_DURATION: 0.74,
                TraceViewHeaderConstant.TRACE_HEADER_PH: "X",
                TraceViewHeaderConstant.TRACE_HEADER_ARGS: {
                    "Task Type": "SDMA", "Stream Id": 11, "Task Id": 4043,
                    "Batch Id": 0, "Subtask Id": 0, "connection_id": 19,
                }
            },
            {
                TraceViewHeaderConstant.TRACE_HEADER_NAME: "ApplyAdamW",
                TraceViewHeaderConstant.TRACE_HEADER_PID: 3,
                TraceViewHeaderConstant.TRACE_HEADER_TID: 4,
                TraceViewHeaderConstant.TRACE_HEADER_TS: 134525368863,
                TraceViewHeaderConstant.TRACE_HEADER_DURATION: 31.92,
                TraceViewHeaderConstant.TRACE_HEADER_PH: "X",
                TraceViewHeaderConstant.TRACE_HEADER_ARGS: {
                    "Task Type": "AI_CORE", "Stream Id": 4, "Task Id": 33573,
                    "Batch Id": 23, "Subtask Id": 4294967295, "connection_id": 14983
                },
            },
        ]
        connection_id = (4 << 64) + (33573 << 48) + (23 << 32) + 4294967295
        expected_data = task_time_data + [
            {
                TraceViewHeaderConstant.TRACE_HEADER_NAME: f"HostToDevice{connection_id}",
                TraceViewHeaderConstant.TRACE_HEADER_PH: "s",
                TraceViewHeaderConstant.TRACE_HEADER_CAT: "HostToDevice",
                TraceViewHeaderConstant.TRACE_HEADER_ID: str(connection_id),
                TraceViewHeaderConstant.TRACE_HEADER_PID: 3,
                TraceViewHeaderConstant.TRACE_HEADER_TID: 2287318,
                TraceViewHeaderConstant.TRACE_HEADER_TS: "34789457752.030"
            },
            {
                TraceViewHeaderConstant.TRACE_HEADER_NAME: f"HostToDevice{connection_id}",
                TraceViewHeaderConstant.TRACE_HEADER_PH: "f",
                TraceViewHeaderConstant.TRACE_HEADER_ID: str(connection_id),
                TraceViewHeaderConstant.TRACE_HEADER_TS: 134525368863,
                TraceViewHeaderConstant.TRACE_HEADER_CAT: "HostToDevice",
                TraceViewHeaderConstant.TRACE_HEADER_PID: 3,
                TraceViewHeaderConstant.TRACE_HEADER_TID: 4,
                TraceViewHeaderConstant.TRACE_HEADER_BP: "e"
            },
        ]
        with mock.patch(NAMESPACE + '.HostToDevice.get_node_tasks',
                        return_value={(4, 33573, 23): (2287318, 4294967295)}), \
                mock.patch(NAMESPACE + ".HostToDevice.get_cann_pid", return_value=3), \
                mock.patch(NAMESPACE + ".HostToDevice.get_connection_id_to_context_ids_mapping",
                           return_value={14983: [4294967295]}):
            InfoConfReader()._host_freq = 123456
            connection = HostToDevice("")
            connection.add_connect_line(task_time_data, "task_time")
            self.assertEqual(task_time_data, expected_data)

    def test_add_connect_line_hccl(self):
        hccl_data = [
            {
                TraceViewHeaderConstant.TRACE_HEADER_NAME: "hcom_broadcast__072_0",
                TraceViewHeaderConstant.TRACE_HEADER_PID: 228731811,
                TraceViewHeaderConstant.TRACE_HEADER_TID: 0,
                TraceViewHeaderConstant.TRACE_HEADER_TS: 134524588800.38,
                TraceViewHeaderConstant.TRACE_HEADER_DURATION: 0.74,
                TraceViewHeaderConstant.TRACE_HEADER_ARGS: {"connection_id": 19},
                TraceViewHeaderConstant.TRACE_HEADER_PH: "X",
            },
        ]
        expected_data = hccl_data + [
            {
                TraceViewHeaderConstant.TRACE_HEADER_NAME: "HostToDevice81604378623",
                TraceViewHeaderConstant.TRACE_HEADER_PH: "f",
                TraceViewHeaderConstant.TRACE_HEADER_ID: "81604378623",
                TraceViewHeaderConstant.TRACE_HEADER_TS: 134524588800.38,
                TraceViewHeaderConstant.TRACE_HEADER_CAT: "HostToDevice",
                TraceViewHeaderConstant.TRACE_HEADER_PID: 228731811,
                TraceViewHeaderConstant.TRACE_HEADER_TID: 0,
                TraceViewHeaderConstant.TRACE_HEADER_BP: "e"
            },
        ]
        with mock.patch(NAMESPACE + '.HostToDevice.get_node_tasks', return_value={(4, 33573, 23), (4, 33574, 23)}), \
                mock.patch(NAMESPACE + ".HostToDevice.get_connection_id_to_context_ids_mapping",
                           return_value={14983: [4294967295]}):
            connection = HostToDevice("")
            connection.add_connect_line(hccl_data, "hccl")
            self.assertEqual(hccl_data, expected_data)

    def test_add_connect_line_should_add_two_line_when_input_one_node_has_two_task(self):
        task_time_data = [
            {
                TraceViewHeaderConstant.TRACE_HEADER_NAME: "Task1",
                TraceViewHeaderConstant.TRACE_HEADER_PID: 3,
                TraceViewHeaderConstant.TRACE_HEADER_TID: 4,
                TraceViewHeaderConstant.TRACE_HEADER_TS: "134525368863",
                TraceViewHeaderConstant.TRACE_HEADER_DURATION: 10, 
                TraceViewHeaderConstant.TRACE_HEADER_PH: "X",
                TraceViewHeaderConstant.TRACE_HEADER_ARGS: {
                    "Task Type": "AI_CORE", "Stream Id": 4, "Task Id": 33573,
                    "Batch Id": 23, "Subtask Id": 1, "connection_id": 14983
                },
            },
            {
                TraceViewHeaderConstant.TRACE_HEADER_NAME: "Task2",
                TraceViewHeaderConstant.TRACE_HEADER_PID: 3,
                TraceViewHeaderConstant.TRACE_HEADER_TID: 4,
                TraceViewHeaderConstant.TRACE_HEADER_TS: "134525369863",
                TraceViewHeaderConstant.TRACE_HEADER_DURATION: 20,
                TraceViewHeaderConstant.TRACE_HEADER_PH: "X",
                TraceViewHeaderConstant.TRACE_HEADER_ARGS: {
                    "Task Type": "AI_CORE", "Stream Id": 4, "Task Id": 33574,
                    "Batch Id": 23, "Subtask Id": 2, "connection_id": 14983
                },
            },
        ]
        task1_conn_id = (4 << 64) + (33573 << 48) + (23 << 32) + 1
        task2_conn_id = (4 << 64) + (33574 << 48) + (23 << 32) + 2
        expected_data = task_time_data + [
            {
                TraceViewHeaderConstant.TRACE_HEADER_NAME: f"HostToDevice{task1_conn_id}",
                TraceViewHeaderConstant.TRACE_HEADER_PH: "s",
                TraceViewHeaderConstant.TRACE_HEADER_CAT: "HostToDevice",
                TraceViewHeaderConstant.TRACE_HEADER_ID: str(task1_conn_id),
                TraceViewHeaderConstant.TRACE_HEADER_PID: 3,
                TraceViewHeaderConstant.TRACE_HEADER_TID: 2287318,
                TraceViewHeaderConstant.TRACE_HEADER_TS: "81000528.403"
            },
            {
                TraceViewHeaderConstant.TRACE_HEADER_NAME: f"HostToDevice{task1_conn_id}",
                TraceViewHeaderConstant.TRACE_HEADER_PH: "f",
                TraceViewHeaderConstant.TRACE_HEADER_ID: str(task1_conn_id),
                TraceViewHeaderConstant.TRACE_HEADER_TS: "134525368863",
                TraceViewHeaderConstant.TRACE_HEADER_CAT: "HostToDevice",
                TraceViewHeaderConstant.TRACE_HEADER_PID: 3,
                TraceViewHeaderConstant.TRACE_HEADER_TID: 4,
                TraceViewHeaderConstant.TRACE_HEADER_BP: "e"
            },
            {
                TraceViewHeaderConstant.TRACE_HEADER_NAME: f"HostToDevice{task2_conn_id}",
                TraceViewHeaderConstant.TRACE_HEADER_PH: "s",
                TraceViewHeaderConstant.TRACE_HEADER_CAT: "HostToDevice",
                TraceViewHeaderConstant.TRACE_HEADER_ID: str(task2_conn_id),
                TraceViewHeaderConstant.TRACE_HEADER_PID: 3,
                TraceViewHeaderConstant.TRACE_HEADER_TID: 2287318,
                TraceViewHeaderConstant.TRACE_HEADER_TS: "162001046.807"
            },
            {
                TraceViewHeaderConstant.TRACE_HEADER_NAME: f"HostToDevice{task2_conn_id}",
                TraceViewHeaderConstant.TRACE_HEADER_PH: "f",
                TraceViewHeaderConstant.TRACE_HEADER_ID: str(task2_conn_id),
                TraceViewHeaderConstant.TRACE_HEADER_TS: "134525369863",
                TraceViewHeaderConstant.TRACE_HEADER_CAT: "HostToDevice",
                TraceViewHeaderConstant.TRACE_HEADER_PID: 3,
                TraceViewHeaderConstant.TRACE_HEADER_TID: 4,
                TraceViewHeaderConstant.TRACE_HEADER_BP: "e"
            },
        ]

        with mock.patch(NAMESPACE + '.HostToDevice.get_node_tasks',
                        return_value={(4, 33573, 23): (2287318, 10000000), (4, 33574, 23): (2287318, 20000000)}), \
                mock.patch(NAMESPACE + ".HostToDevice.get_cann_pid", return_value=3):
            InfoConfReader()._host_freq = 123456
            connection = HostToDevice("")
            connection.add_connect_line(task_time_data, "task_time")
            self.assertEqual(task_time_data, expected_data)

    def test_get_cann_pid(self):
        InfoConfReader()._info_json = {TraceViewHeaderConstant.TRACE_HEADER_PID: 123}
        cann_pid = HostToDevice.get_cann_pid()
        self.assertEqual(cann_pid, 126047)

    def test_get_hccl_op_connection_ids(self):
        with mock.patch("common_func.msvp_common.path_check", return_value=False):
            connection = HostToDevice("")
            ret = connection.get_hccl_op_connection_ids()
            self.assertEqual(ret, set())

#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.

from collections import defaultdict
from typing import Any, Dict, List, Set, Tuple

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msvp_common import path_check
from common_func.path_manager import PathManager
from msmodel.interface.view_model import ViewModel


class HostToDevice:
    """Connect CANN Node@launch api to corresponding device tasks/HCCL OP."""
    API_TYPE = 'api'
    MODULE_TASK_TIME = 'task_time'
    MODULE_HCCL = 'hccl'
    NODE_LAUNCH = "Node@launch"

    def __init__(self, result_dir: str) -> None:
        self._result_dir = result_dir

    @staticmethod
    def is_node_launch(api_trace: Dict[str, Any]) -> bool:
        """
        check if some trace is the start of flow event, that is, it's Node@launch
        :param api_trace: api trace as json
        :return: bool
        """
        return api_trace.get("name") == HostToDevice.NODE_LAUNCH

    @staticmethod
    def get_start_points(api_trace: Dict[str, Any], conn_to_ctxes: Dict[int, List[int]]) -> List[Dict[str, Any]]:
        """
        calculate start points of host to device connection for a single api trace
        :param api_trace: api trace as json
        :param task_info_reversed: reversed task info list
        :return: start point
        """
        start_time = api_trace.get('ts', '0')
        connection_id = api_trace.get("args", {}).get("connection_id", Constant.DEFAULT_INVALID_VALUE)
        context_ids = conn_to_ctxes.get(connection_id, [Constant.DEFAULT_INVALID_VALUE])
        return [
            {
                'name': f'HostToDevice{(connection_id << 32) + ctx_id}', 'ph': 's',
                'cat': StrConstant.HOST_TO_DEVICE, 'id': str((connection_id << 32) + ctx_id),
                'pid': api_trace.get('pid'), 'tid': api_trace.get('tid'), 'ts': start_time
            }
            for ctx_id in context_ids
        ]

    @staticmethod
    def add_task_end_points(traces: List[Dict[str, Any]], node_tasks: Set[Tuple[int, int, int]]) -> None:
        """
        add end points for host to device connection
        :param task_traces: task traces as json list
        :return: None
        """
        if not isinstance(traces, list):
            return
        tmp_list = []
        for trace in traces:
            trace_args = trace.get('args', {})
            connection_id = trace_args.get('connection_id', Constant.DEFAULT_INVALID_VALUE)
            if connection_id == Constant.DEFAULT_INVALID_VALUE:
                continue
            stream_id = trace_args.get("Stream Id")
            task_id = trace_args.get("Task Id")
            batch_id = trace_args.get("Batch Id")
            context_id: int = trace_args.get("Subtask Id", Constant.DEFAULT_INVALID_VALUE)
            if (stream_id, task_id, batch_id) not in node_tasks:
                continue
            pid, tid = trace.get('pid'), trace.get('tid')
            connect_dict = {
                'name': f'HostToDevice{(connection_id << 32) + context_id}', 'ph': 'f',
                'id': str((connection_id << 32) + context_id), 'ts': trace.get('ts'),
                'cat': StrConstant.HOST_TO_DEVICE, 'pid': pid, 'tid': tid, 'bp': 'e',
            }
            tmp_list.append(connect_dict)
        traces.extend(tmp_list)

    @staticmethod
    def add_hccl_end_points(traces: List[Dict[str, Any]]) -> None:
        """
        add end points for host to device connection
        :param task_traces: hccl traces as json list
        :return: None
        """
        if not isinstance(traces, list):
            return
        tmp_list = []
        for trace in traces:
            trace_args = trace.get('args', {})
            connection_id = trace_args.get('connection_id', Constant.DEFAULT_INVALID_VALUE)
            if connection_id == Constant.DEFAULT_INVALID_VALUE:
                continue
            context_id: int = trace_args.get("Subtask Id", Constant.DEFAULT_INVALID_VALUE)
            pid = trace.get('pid')
            tid = trace.get('tid')
            connect_dict = {
                'name': f'HostToDevice{(connection_id << 32) + context_id}', 'ph': 'f',
                'id': str((connection_id << 32) + context_id), 'ts': trace.get('ts'),
                'cat': StrConstant.HOST_TO_DEVICE, 'pid': pid, 'tid': tid, 'bp': 'e',
            }
            tmp_list.append(connect_dict)
        traces.extend(tmp_list)

    def add_start_points(self, api_traces: List[Dict[str, Any]], conn_to_ctxes: Dict[int, List[int]]) -> None:
        """
        add start points to api traces for host to device connection
        to do this, we need task info from host side
        this is bad design BTW
        :param api_traces: api traces as json list
        :return: None
        """
        if not isinstance(api_traces, list):
            return
        tmp_list = []
        for api_trace in api_traces:
            if not HostToDevice.is_node_launch(api_trace):
                continue
            start_point = self.get_start_points(api_trace, conn_to_ctxes)
            tmp_list.extend(start_point)
        api_traces.extend(tmp_list)

    def add_connect_line(self, traces: List[Dict[str, Any]], data_type: str) -> None:
        """
        connect traces from Node@launch to device tasks
        add start points in api export, end points in task_time export
        :param traces: json traces
        :param data_type: export type
        """
        node_tasks = self.get_node_tasks()
        if data_type == self.API_TYPE:
            conn_to_ctxes = self.get_connection_id_to_context_ids_mapping(node_tasks)
            self.add_start_points(traces, conn_to_ctxes)
        if data_type == self.MODULE_TASK_TIME:
            self.add_task_end_points(traces, node_tasks)
        if data_type == self.MODULE_HCCL:
            self.add_hccl_end_points(traces)

    def get_node_tasks(self) -> Set[Tuple[int, int, int]]:
        """
        get node tasks set
        :return: node tasks set
        """
        if not path_check(PathManager.get_db_path(self._result_dir, DBNameConstant.DB_GE_INFO)):
            return set()
        task_info_model = ViewModel(self._result_dir, DBNameConstant.DB_GE_INFO, [DBNameConstant.TABLE_GE_TASK])
        task_info_model.init()
        sql = 'select stream_id, task_id, batch_id from TaskInfo'
        tasks = task_info_model.get_sql_data(sql)
        return set(tuple(task) for task in tasks)

    def get_connection_id_to_context_ids_mapping(self, node_tasks: Set[Tuple[int, int, int]]) -> Dict[int, List[int]]:
        """
        get device tasks
        :return: device tasks
        """
        if not path_check(PathManager.get_db_path(self._result_dir, DBNameConstant.DB_ASCEND_TASK)):
            return {}
        ascend_task_model = ViewModel(self._result_dir, DBNameConstant.DB_ASCEND_TASK,
                                      [DBNameConstant.TABLE_ASCEND_TASK])
        ascend_task_model.init()
        sql = 'select stream_id, task_id, batch_id, context_id, connection_id from AscendTask'
        ascend_tasks = ascend_task_model.get_sql_data(sql)
        result = defaultdict(list)
        for stream_id, task_id, batch_id, context_id, connection_id in ascend_tasks:
            if (stream_id, task_id, batch_id) not in node_tasks:
                continue
            result[connection_id].append(context_id)
        return result

#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.

import json
from typing import Dict
from typing import List

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.stars_constant import StarsConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from mscalculate.ascend_task.ascend_task import TopDownTask
from msmodel.interface.view_model import ViewModel
from msmodel.stars.ffts_log_model import FftsLogModel
from msmodel.stars.sub_task_model import SubTaskTimeModel
from msmodel.task_time.ascend_task_model import AscendTaskModel
from profiling_bean.db_dto.ge_task_dto import GeTaskDto
from profiling_bean.db_dto.task_time_dto import TaskTimeDto
from profiling_bean.prof_enum.export_data_type import ExportDataType
from viewer.get_trace_timeline import TraceViewer
from viewer.interface.base_viewer import BaseViewer
from viewer.memory_copy.memory_copy_viewer import MemoryCopyViewer


class TaskTimeViewer(BaseViewer):
    """
    class for get task time data
    """

    TRACE_PID_MAP = {
        TraceViewHeaderConstant.PROCESS_TASK: 0,
        TraceViewHeaderConstant.PROCESS_SUBTASK: 1,
        TraceViewHeaderConstant.PROCESS_THREAD_TASK: 2
    }

    def __init__(self: any, configs: dict, params: dict) -> None:
        super().__init__(configs, params)
        self.project_dir = self.params.get(StrConstant.PARAM_RESULT_DIR)

    def get_time_timeline_header(self: any, data: list, pid_header=TraceViewHeaderConstant.PROCESS_TASK) -> list:
        """
        to get sequence chrome trace json header
        :return: header of trace data list
        """
        header = [
            [
                "process_name", self.TRACE_PID_MAP.get(pid_header, 0),
                InfoConfReader().get_json_tid_data(), pid_header
            ]
        ]
        subtask = []
        tid_set = set((item[1], item[2]) for item in data)
        for item in tid_set:
            if pid_header == TraceViewHeaderConstant.PROCESS_THREAD_TASK:
                thread_id = int(item[1] / (max(StarsConstant.SUBTASK_TYPE) + 1))
                device_task_type = StarsConstant.SUBTASK_TYPE.get(item[1] % (max(StarsConstant.SUBTASK_TYPE) + 1))
                subtask.append(["thread_name", item[0], item[1], f'Thread {thread_id}({device_task_type})'])
            elif pid_header == TraceViewHeaderConstant.PROCESS_SUBTASK:
                subtask.append(["thread_name", item[0], item[1], f'Stream {StarsConstant.SUBTASK_TYPE.get(item[1])}'])
            else:
                subtask.append(["thread_name", item[0], item[1], f'Stream {item[1]}'])
            subtask.append(["thread_sort_index", item[0], item[1], item[1]])
        header.extend(subtask)
        return header

    def get_ascend_task_data(self: any) -> Dict[str, List[TopDownTask]]:
        task_data = {
            'task_data_list': [],
            'subtask_data_list': [],
        }
        with AscendTaskModel(self.project_dir, [DBNameConstant.TABLE_ASCEND_TASK]) as model:
            task_data_list = model.get_ascend_task_data_without_unknown()
        for data in task_data_list:
            if data.context_id == NumberConstant.DEFAULT_GE_CONTEXT_ID:
                task_data['task_data_list'].append(data)
            else:
                task_data['subtask_data_list'].append(data)
        return task_data

    def get_timeline_data(self: any) -> str:
        """
        get model list timeline data
        @return:timeline trace data
        """
        result = []
        result_dir = self.params.get(StrConstant.PARAM_RESULT_DIR)
        # add memory copy data
        memory_copy_viewer = MemoryCopyViewer(result_dir)
        trace_data_memcpy = memory_copy_viewer.get_memory_copy_timeline()
        result.extend(trace_data_memcpy)

        timeline_data = self.get_ascend_task_data()
        trace_tasks = self.get_trace_timeline(timeline_data)
        result.extend(trace_tasks)
        if not result:
            return json.dumps({
                "status": NumberConstant.WARN,
                "info": "Can not export task time data, the current chip does not support "
                        "exporting this data or the data may be not collected."
            })
        return TraceViewer("StarsViewer").format_trace_events(result)

    def get_model_instance(self: any) -> any:
        """
        get model instance from list
        """
        return FftsLogModel(self.params.get(StrConstant.PARAM_RESULT_DIR), DBNameConstant.DB_SOC_LOG, [])

    def get_trace_timeline(self: any, data_list: dict) -> list:
        """
        to format data to chrome trace json
        :return: timeline_trace list
        """
        if not data_list or not any(data_list.values()):
            return []
        self.add_node_name(data_list)
        if self.params.get("data_type") == ExportDataType.FFTS_SUB_TASK_TIME.name.lower():
            self.add_thread_id(data_list)
            return self.format_ffts_sub_task_data(data_list)
        return self.format_task_scheduler(data_list)

    def format_task_type_data(self, data_list):
        result_list = []
        for data in data_list.get("subtask_data_list", []):
            result_list.append(
                [data.op_name,
                 self.TRACE_PID_MAP.get("Subtask Time", 1),
                 StarsConstant().find_key_by_value(data.device_task_type),
                 InfoConfReader().trans_into_local_time(data.start_time, NumberConstant.NANO_SECOND),
                 data.duration / DBManager.NSTOUS if data.duration > 0 else 0,
                 {"Stream Id": data.stream_id, "Task Id": data.task_id, 'Batch Id': data.batch_id,
                  "Subtask Id": data.context_id, "connection_id": data.connection_id, }])
        if not result_list:
            return []
        _trace = TraceViewManager.time_graph_trace(TraceViewHeaderConstant.TOP_DOWN_TIME_GRAPH_HEAD,
                                                   result_list)
        result = TraceViewManager.metadata_event(
            self.get_time_timeline_header(result_list, pid_header=TraceViewHeaderConstant.PROCESS_SUBTASK))
        result.extend(_trace)
        return result

    def format_thread_task_data(self, data_list):
        result_list = []
        for data in data_list.get("subtask_data_list", []):
            result_list.append(
                [data.op_name,
                 self.TRACE_PID_MAP.get("Thread Task Time", 2),
                 data.thread_id * (max(StarsConstant.SUBTASK_TYPE) + 1) + \
                 StarsConstant().find_key_by_value(data.device_task_type),
                 InfoConfReader().trans_into_local_time(data.start_time, NumberConstant.NANO_SECOND),
                 data.duration / DBManager.NSTOUS if data.duration > 0 else 0,
                 {"Stream Id": data.stream_id, "Task Id": data.task_id, 'Batch Id': data.batch_id,
                  "Subtask Id": data.context_id, "Subtask Type": data.device_task_type,
                  "connection_id": data.connection_id, }])
        if not result_list:
            return []
        _trace = TraceViewManager.time_graph_trace(TraceViewHeaderConstant.TOP_DOWN_TIME_GRAPH_HEAD,
                                                   result_list)
        result = TraceViewManager.metadata_event(
            self.get_time_timeline_header(result_list, pid_header=TraceViewHeaderConstant.PROCESS_THREAD_TASK))
        result.extend(_trace)
        return result

    def format_task_scheduler(self, data_list):
        result_list = []
        for data in data_list.get("subtask_data_list", []):
            result_list.append(
                [data.op_name,
                 self.TRACE_PID_MAP.get("Task Scheduler", 0),
                 data.stream_id,
                 InfoConfReader().trans_into_local_time(data.start_time, NumberConstant.NANO_SECOND),
                 data.duration / DBManager.NSTOUS if data.duration > 0 else 0,
                 {"Task Type": data.device_task_type, "Stream Id": data.stream_id, "Task Id": data.task_id,
                  'Batch Id': data.batch_id, "Subtask Id": data.context_id, "connection_id": data.connection_id, }])
        for data in data_list.get("task_data_list", []):
            result_list.append(
                [data.op_name,
                 self.TRACE_PID_MAP.get("Task Scheduler", 0),
                 data.stream_id,
                 InfoConfReader().trans_into_local_time(data.start_time, NumberConstant.NANO_SECOND),
                 data.duration / DBManager.NSTOUS if data.duration > 0 else 0,
                 {"Task Type": data.device_task_type, "Stream Id": data.stream_id, "Task Id": data.task_id,
                  'Batch Id': data.batch_id, "Subtask Id": data.context_id, "connection_id": data.connection_id, }])
        if not result_list:
            return []
        _trace = TraceViewManager.time_graph_trace(TraceViewHeaderConstant.TOP_DOWN_TIME_GRAPH_HEAD,
                                                   result_list)
        result = TraceViewManager.metadata_event(self.get_time_timeline_header(result_list))
        result.extend(_trace)
        return result

    def format_ffts_sub_task_data(self, data_list):
        return self.format_thread_task_data(data_list) + self.format_task_type_data(data_list)

    def add_thread_id(self: any, data_dict: dict) -> None:
        with SubTaskTimeModel(self.project_dir) as model:
            subtask_data = model.get_all_data(DBNameConstant.TABLE_SUBTASK_TIME, TaskTimeDto)
        thread_id_dict = {}
        for data in subtask_data:
            key = "{0}-{1}-{2}-{3}".format(data.task_id, data.stream_id, data.subtask_id, data.start_time)
            thread_id_dict[key] = data.thread_id
        for data in data_dict.get("subtask_data_list", []):
            thread_id_key = "{0}-{1}-{2}-{3}".format(data.task_id, data.stream_id, data.context_id, data.start_time)
            setattr(data, 'thread_id', thread_id_dict.get(thread_id_key))

    def add_node_name(self: any, data_dict: dict) -> None:
        node_name_dict, task_type_dict = self.get_ge_data_dict()
        ffts_plus_set = set()
        for data in data_dict.get("subtask_data_list", []):
            ffts_plus_set.add("{0}-{1}-{2}-{3}".format(
                data.task_id, data.stream_id, NumberConstant.DEFAULT_GE_CONTEXT_ID, data.batch_id))
            node_key = "{0}-{1}-{2}-{3}".format(data.task_id, data.stream_id, data.context_id, data.batch_id)
            setattr(data, 'op_name', node_name_dict.get(node_key, data.device_task_type))
        tradition_list = []
        for data in data_dict.get("task_data_list", []):
            node_key = "{0}-{1}-{2}-{3}".format(
                data.task_id, data.stream_id, NumberConstant.DEFAULT_GE_CONTEXT_ID, data.batch_id)
            if node_key not in ffts_plus_set:
                setattr(data, 'op_name', node_name_dict.get(node_key, data.device_task_type))
                tradition_list.append(data)
        data_dict["task_data_list"] = tradition_list

    def get_ge_data_dict(self: any) -> tuple:
        node_dict, task_type_dict = {}, {}
        view_model = ViewModel(self.params.get("project"), DBNameConstant.DB_AICORE_OP_SUMMARY,
                               DBNameConstant.TABLE_GE_TASK)
        view_model.init()
        ge_data = view_model.get_all_data(DBNameConstant.TABLE_SUMMARY_GE, dto_class=GeTaskDto)
        for data in ge_data:
            node_key = "{0}-{1}-{2}-{3}".format(data.task_id, data.stream_id, data.context_id, data.batch_id)
            node_dict[node_key] = data.op_name
            if data.context_id == NumberConstant.DEFAULT_GE_CONTEXT_ID:
                task_type_dict[node_key] = data.task_type
        return node_dict, task_type_dict

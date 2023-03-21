#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.


from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.stars_constant import StarsConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from msmodel.interface.view_model import ViewModel
from msmodel.stars.ffts_log_model import FftsLogModel
from profiling_bean.db_dto.ge_task_dto import GeTaskDto
from profiling_bean.prof_enum.export_data_type import ExportDataType
from viewer.get_trace_timeline import TraceViewer
from viewer.interface.base_viewer import BaseViewer


class FftsLogViewer(BaseViewer):
    """
    class for get ffts_log data
    """

    TRACE_PID_MAP = {
        TraceViewHeaderConstant.PROCESS_TASK: 0,
        TraceViewHeaderConstant.PROCESS_SUBTASK: 1,
        TraceViewHeaderConstant.PROCESS_THREAD_TASK: 2
    }

    def __init__(self: any, configs: dict, params: dict) -> None:
        super().__init__(configs, params)

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
            subtask.append(
                ["thread_name", item[0], item[1],
                 StarsConstant.SUBTASK_TYPE.get(item[1], item[1])])
        header.extend(subtask)
        return header

    def get_timeline_data(self: any) -> str:
        """
        get model list timeline data
        @return:timeline trace data
        """
        timeline_data = self.get_data_from_db()
        result = self.get_trace_timeline(timeline_data)
        if not result:
            result = {
                "status": NumberConstant.WARN,
                "info": "Can not export ffts sub task time data, the ffts switch may be set to OFF."
            }
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
        if not any(data_list.values()):
            return []
        self.add_node_name(data_list)
        if self.params.get("data_type") == ExportDataType.FFTS_SUB_TASK_TIME.name.lower():
            return self.format_ffts_sub_task_data(data_list)
        return self.format_task_scheduler(data_list)

    def format_task_type_data(self, data_list):
        result_list = []
        for data in data_list.get("subtask_data_list", []):
            result_list.append(
                [data.op_name,
                 self.TRACE_PID_MAP.get("Subtask Time", 1),
                 data.subtask_type,
                 data.start_time / DBManager.NSTOUS,
                 data.dur_time / DBManager.NSTOUS if data.dur_time > 0 else 0,
                 {"FFTS Type": data.ffts_type, "Stream Id": data.stream_id, "Task Id": data.task_id,
                  "Subtask Id": data.subtask_id}])
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
        for data in data_list.get("thread_data_list", []):
            result_list.append(
                [data.op_name,
                 self.TRACE_PID_MAP.get("Thread Task Time", 2),
                 "Thread {0}({1})".format(str(data.thread_id), data.subtask_type),
                 data.task_time / DBManager.NSTOUS,
                 data.dur_time / DBManager.NSTOUS if data.dur_time > 0 else 0,
                 {"FFTS Type": data.ffts_type, "Stream Id": data.stream_id, "Task Id": data.task_id,
                  "Subtask Id": data.subtask_id, "Subtask Type": data.subtask_type}])
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
                 "Stream {}".format(str(data.stream_id)),
                 data.start_time / DBManager.NSTOUS,
                 data.dur_time / DBManager.NSTOUS if data.dur_time > 0 else 0,
                 {"FFTS Type": data.ffts_type, "Task Type": data.subtask_type, "Stream Id": data.stream_id,
                  "Task Id": data.task_id, "Subtask Id": data.subtask_id}])
        for data in data_list.get("acsq_task_list", []):
            result_list.append(
                [data.op_name,
                 self.TRACE_PID_MAP.get("Task Scheduler", 0),
                 "Stream {}".format(str(data.stream_id)),
                 data.start_time / DBManager.NSTOUS,
                 data.task_time / DBManager.NSTOUS if data.task_time > 0 else 0,
                 {"Task Type": data.task_type, "Stream Id": data.stream_id,
                  "Task Id": data.task_id, "Subtask Id": data.subtask_id}])
        if not result_list:
            return []
        _trace = TraceViewManager.time_graph_trace(TraceViewHeaderConstant.TOP_DOWN_TIME_GRAPH_HEAD,
                                                   result_list)
        result = TraceViewManager.metadata_event(self.get_time_timeline_header(result_list))
        result.extend(_trace)
        return result

    def format_ffts_sub_task_data(self, data_list):
        return self.format_thread_task_data(data_list) + self.format_task_type_data(data_list)

    def add_node_name(self: any, data_dict: dict) -> None:
        node_name_dict, task_type_dict = self.get_ge_data_dict()
        ffts_plus_set = set()
        for data in data_dict.get("subtask_data_list", []):
            ffts_plus_set.add("{0}-{1}-{2}".format(data.task_id, data.stream_id, NumberConstant.DEFAULT_GE_CONTEXT_ID))
            node_key = "{0}-{1}-{2}".format(data.task_id, data.stream_id, data.subtask_id)
            data.op_name = node_name_dict.get(node_key, data.subtask_type)
        for data in data_dict.get("thread_data_list", []):
            node_key = "{0}-{1}-{2}".format(data.task_id, data.stream_id, data.subtask_id)
            data.op_name = node_name_dict.get(node_key, data.subtask_type)
        tradition_list = []
        for data in data_dict.get("acsq_task_list", []):
            node_key = "{0}-{1}-{2}".format(data.task_id, data.stream_id, NumberConstant.DEFAULT_GE_CONTEXT_ID)
            if node_key not in ffts_plus_set:
                data.task_type = task_type_dict.get(node_key, data.task_type)
                data.op_name = node_name_dict.get(node_key, data.task_type)
                tradition_list.append(data)
        data_dict["acsq_task_list"] = tradition_list

    def get_ge_data_dict(self: any) -> tuple:
        node_dict, task_type_dict = {}, {}
        view_model = ViewModel(self.params.get("project"), DBNameConstant.DB_AICORE_OP_SUMMARY,
                               DBNameConstant.TABLE_GE_TASK)
        view_model.init()
        ge_data = view_model.get_all_data(DBNameConstant.TABLE_SUMMARY_GE, dto_class=GeTaskDto)
        for data in ge_data:
            node_key = "{0}-{1}-{2}".format(data.task_id, data.stream_id, data.context_id)
            node_dict[node_key] = data.op_name
            if data.context_id == NumberConstant.DEFAULT_GE_CONTEXT_ID:
                task_type_dict[node_key] = data.task_type
        return node_dict, task_type_dict

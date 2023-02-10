#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.


from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.stars_constant import StarsConstant
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from msmodel.interface.view_model import ViewModel
from msmodel.stars.ffts_log_model import FftsLogModel
from profiling_bean.db_dto.ge_task_dto import GeTaskDto
from profiling_bean.prof_enum.export_data_type import ExportDataType
from viewer.interface.base_viewer import BaseViewer


class FftsLogViewer(BaseViewer):
    """
    class for get ffts_log data
    """
    SUBTASK_TIME = 'Subtask Time'

    def __init__(self: any, configs: dict, params: dict) -> None:
        super().__init__(configs, params)
        self.model_list = {
            ExportDataType.FFTS_SUB_TASK_TIME.name.lower(): FftsLogModel,
        }

    def get_time_timeline_header(self: any, data: list) -> list:
        """
        to get sequence chrome trace json header
        :return: header of trace data list
        """
        header = [
            [
                "process_name", InfoConfReader().get_json_pid_data(),
                InfoConfReader().get_json_tid_data(), self.SUBTASK_TIME
            ]
        ]
        subtask = []
        for item in data:
            subtask.append(
                ["thread_name", item[1], item[2],
                 StarsConstant.SUBTASK_TYPE.get(item[2], item[2])])
        header.extend(subtask)
        return header

    def get_trace_timeline(self: any, data_list: dict) -> list:
        """
        to format data to chrome trace json
        :return: timeline_trace list
        """
        if not data_list:
            return []
        data_list = self.add_node_name(data_list)
        result = []
        for data in data_list.get('subtask_data_list', []):
            result.append(
                [data.op_name,
                 InfoConfReader().get_json_pid_data(),
                 data.subtask_type,  # subtask type
                 data.start_time / DBManager.NSTOUS,  # start time
                 data.dur_time / DBManager.NSTOUS if data.dur_time > 0 else 0,  # duration
                 {'FFTS Type': data.ffts_type, 'Stream ID': data.stream_id, 'Task ID': data.task_id,
                  'Subtask_id': data.subtask_id}])
        _trace = TraceViewManager.time_graph_trace(TraceViewHeaderConstant.TOP_DOWN_TIME_GRAPH_HEAD,
                                                   result)
        result = TraceViewManager.metadata_event(self.get_time_timeline_header(result))
        result.extend(_trace)
        return result

    def add_node_name(self: any, data_dict: dict) -> dict:
        node_name_dict = self.get_node_name()
        for data in data_dict.get('subtask_data_list', []):
            node_key = "{0}-{1}-{2}".format(data.task_id, data.stream_id, data.subtask_id)
            data.op_name = node_name_dict.get(node_key, 'NA')
        return data_dict

    def get_node_name(self: any) -> dict:
        node_dict = {}
        view_model = ViewModel(self.params.get('project'), DBNameConstant.DB_AICORE_OP_SUMMARY,
                               DBNameConstant.TABLE_GE_TASK)
        view_model.init()
        ge_data = view_model.get_all_data(DBNameConstant.TABLE_SUMMARY_GE, dto_class=GeTaskDto)
        for data in ge_data:
            node_key = "{0}-{1}-{2}".format(data.task_id, data.stream_id, data.context_id)
            node_dict[node_key] = data.op_name
        return node_dict

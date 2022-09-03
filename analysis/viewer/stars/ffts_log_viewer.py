#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.

from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.stars_constant import StarsConstant
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from msmodel.stars.ffts_log_model import FftsLogModel
from profiling_bean.prof_enum.export_data_type import ExportDataType
from viewer.interface.base_viewer import BaseViewer


class FftsLogViewer(BaseViewer):
    """
    class for get ffts_log data
    """
    SUBTASK_TIME = 'subtask_time'

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
        header = [["process_name", InfoConfReader().get_json_pid_data(),
                   InfoConfReader().get_json_tid_data(), self.SUBTASK_TIME]]
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
        result = []
        for data in data_list.get('subtask_data_list', []):
            result.append(
                ["subtask_{}".format(str(data[0])),
                 InfoConfReader().get_json_pid_data(),
                 data[3],  # subtask type
                 data[5],  # start time
                 data[6],  # duration
                 {'FFTS Type': data[4], 'Stream ID': data[2], 'Task ID': data[1]}])
        _trace = TraceViewManager.time_graph_trace(TraceViewHeaderConstant.TOP_DOWN_TIME_GRAPH_HEAD,
                                                   result)
        result = TraceViewManager.metadata_event(self.get_time_timeline_header(result))
        result.extend(_trace)
        return result

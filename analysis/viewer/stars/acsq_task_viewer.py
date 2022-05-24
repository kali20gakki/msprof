#!usr/bin/env python
# coding:utf-8
"""
acsq task model
Copyright Huawei Technologies Co., Ltd. 2021. All rights reserved.
"""

from abc import ABC

from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from model.sqe_type_map import SqeType
from model.stars.acsq_task_model import AcsqTaskModel
from viewer.interface.base_viewer import BaseViewer


class AcsqTaskViewer(BaseViewer, ABC):
    """
    class used to get acsq task timeline and op_summary data
    """

    def __init__(self: any, configs: dict, params: dict) -> None:
        super().__init__(configs, params)
        self.model_list = {
            'acsq_task_statistic': AcsqTaskModel,
            'acsq_task_time': AcsqTaskModel,
        }

    @staticmethod
    def get_timeline_header() -> list:
        pid = InfoConfReader().get_json_pid_data()
        result = [["process_name", pid, InfoConfReader().get_json_tid_data(), "AcsqTask"]]

        for sqe in SqeType:
            thread_header = ["thread_name", pid, sqe.value, sqe.name]
            result.append(thread_header)
        return result

    def get_summary_data(self: any) -> tuple:
        """
        get acsq task op_summary data,
        :return: headers, data, count of data
        """
        summary_data_list = list(map(list, self.get_data_from_db()))
        for data in summary_data_list:
            data[4:6] = list(map(InfoConfReader().time_from_syscnt, data[4:6]))
            data[6] = data[5] - data[4]
        return self.configs.get(StrConstant.CONFIG_HEADERS), summary_data_list, len(summary_data_list)

    def get_trace_timeline(self: any, data_list: list) -> list:
        """
        get time timeline
        :return: timeline_trace data
        """
        result = []
        pid = InfoConfReader().get_json_pid_data()
        for data in data_list:
            start_time = InfoConfReader().time_from_syscnt(data[4], NumberConstant.MICRO_SECOND)
            task_dur = InfoConfReader().time_from_syscnt(data[5], NumberConstant.MICRO_SECOND) - start_time
            task_name = "{} {}".format(str(data[1]), str(data[0]))
            result.append([task_name, pid, SqeType[data[3]].value, start_time, task_dur])
        _trace = TraceViewManager.time_graph_trace(TraceViewHeaderConstant.TASK_TIME_GRAPH_HEAD, result)
        result = TraceViewManager.metadata_event(AcsqTaskViewer.get_timeline_header())
        result.extend(_trace)
        return result


#!/usr/bin/python3
# coding:utf-8
"""
This scripts is used to export summary timeline
Copyright Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
"""

import json
import os

from common_func.common import error
from common_func.empty_class import EmptyClass
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_iteration import MsprofIteration
from common_func.singleton import singleton
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from profiling_bean.prof_enum.export_data_type import ExportDataType
from viewer.association.acl_connect_hwts import AclToHwts
from viewer.training.step_trace_viewer import StepTraceViewer


@singleton
class MsprofTimeline:
    """
    This class is used to export a summary timeline json file.
    """
    CONNECT_LIST = [AclToHwts]
    FILE_NAME = os.path.basename(__file__)

    def __init__(self: any) -> None:
        self._iteration_id = 0
        self._index_id = 0
        self._model_id = NumberConstant.DEFAULT_MODEL_ID
        self._result_dir = None
        self._export_data_list = []
        self._iteration_time = []

    def add_export_data(self: any, data: str, data_type: str) -> None:
        """
        index events in bulk json
        :param data_type: data type
        :param data: data
        :param data_type: data_type
        :return: None
        """
        try:
            json_list = json.loads(data)
        except (TypeError, ValueError) as err:
            error(self.FILE_NAME, err)
        else:
            try:
                if isinstance(json_list, list) and json_list:
                    self.add_sort_index(json_list, data_type)
                    self.add_connect_json_line(json_list, data_type)
                    json_list = filter(
                        lambda value: value["ph"] == "M" or self.is_in_iteration(value["ts"]),
                        json_list)
                    self._export_data_list.extend(json_list)
            except (TypeError, ValueError) as err:
                error(self.FILE_NAME, err)

    def add_connect_json_line(self: any, json_list: list, data_type: str) -> None:
        """
        add connect line with task time
        :param json_list: json list
        :param data_type: data_type
        """
        for connect_obj in self.CONNECT_LIST:
            connect_obj(self._result_dir).add_connect_line(json_list, data_type)

    def add_sort_index(self: any, json_list: list, data_type: str) -> None:
        """
        add sort index and header
        :param data_type: data type
        :param json_list: json list
        """
        if isinstance(json_list, list):
            pid_list = set()
            for value in json_list:
                # json_list contains different type data
                format_pid = "{}_{}".format(getattr(ExportDataType, data_type.upper()).value,
                                            value.get(StrConstant.TRACE_HEADER_PID,
                                                      TraceViewHeaderConstant.DEFAULT_PID_VALUE))
                if value.get(StrConstant.TRACE_HEADER_NAME, "") == "process_name":
                    pid_list.add(format_pid)
                value[StrConstant.TRACE_HEADER_PID] = format_pid
            for pid in pid_list:
                json_list.extend(
                    self.get_timeline_header(pid, getattr(ExportDataType, data_type.upper()).value))

    def export_all_data(self: any) -> str:
        """
        get bulk data
        :return: json for timeline
        """
        data = StepTraceViewer.get_one_iter_timeline_data(self._result_dir,
                                                          self._index_id,
                                                          self._model_id)
        if not isinstance(data, EmptyClass):
            data_list = json.loads(data)
            if isinstance(data_list, list) and data_list:
                self.add_sort_index(data_list, ExportDataType.STEP_TRACE.name.lower())
                data_list.extend(self._export_data_list)
                return json.dumps(data_list)
        return json.dumps(self._export_data_list)

    @classmethod
    def get_timeline_header(cls: any, pid: str, pid_sort_index: int) -> list:
        """
        get timeline header
        """
        header = [["process_sort_index", pid,
                   InfoConfReader().get_json_tid_data(), pid_sort_index]]
        process_index = TraceViewManager.metadata_event(header)
        return process_index

    def is_in_iteration(self: any, time_stamp: int) -> bool:
        """
        check if in iteration
        """
        # Show all data without iteration time
        if not self._iteration_time:
            return True
        for start_time, end_time in self._iteration_time:
            if time_stamp < start_time or time_stamp >= end_time:
                return False
        return True

    def set_iteration_info(self: any, result_dir: str, index_id: int, model_id: int) -> None:
        """
        get iteration time
        """
        iteration = MsprofIteration(result_dir)
        self._export_data_list = []
        self._result_dir = result_dir
        self._index_id = index_id
        self._model_id = model_id
        self._iteration_time = iteration.get_iteration_time(index_id, model_id)
        self._iteration_id = iteration.get_iteration_id_by_index_id(index_id, model_id)

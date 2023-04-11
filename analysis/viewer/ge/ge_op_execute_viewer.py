#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

import json
import logging

from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msvp_constant import MsvpConstant
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from msmodel.ge.ge_op_execute_view_model import GeOpExecuteViewModel
from viewer.ge_info_report import get_ge_hash_dict


class GeOpExecuteViewer:
    """
    view for ge op execute
    """

    def __init__(self: any, configs: dict, params: dict) -> None:
        self._configs = configs
        self._params = params
        self._project_path = params.get(StrConstant.PARAM_RESULT_DIR)
        self._model = GeOpExecuteViewModel(params)

    def get_summary_data(self: any) -> tuple:
        """
        get summary data from ge op execute
        :return: summary data
        """
        if not self._model.init():
            logging.error("Ge op_execute maybe parse failed, please check the data parsing log.")
            return MsvpConstant.MSVP_EMPTY_DATA
        summary_data = self._model.get_summary_data()
        if summary_data:
            summary_data = self.update_dynamic_info(summary_data)
            return self._configs.get(StrConstant.CONFIG_HEADERS), summary_data, len(summary_data)
        return MsvpConstant.MSVP_EMPTY_DATA

    def get_timeline_data(self: any) -> str:
        """
        get timeline data from ge op execute
        :return:
        """
        if not self._model.init():
            logging.error("Ge op_execute maybe parse failed, please check the data parsing log.")
            return json.dumps({"status": NumberConstant.ERROR, "info": "No data found for ge op execute."})
        timeline_data = self._model.get_timeline_data()
        timeline_data = self.update_dynamic_info(timeline_data)

        result = []
        pid = InfoConfReader().get_json_pid_data()
        header = [["process_name", pid, InfoConfReader().get_json_tid_data(), TraceViewHeaderConstant.PROCESS_GE]]
        tid_set = set()
        for data in timeline_data:
            _name = data[3] if not data[2] else "{}_{}".format(data[2], str(data[3]))
            result.append([_name, pid, data[0], data[4], data[5]])
            tid_set.add(data[0])
        _trace = TraceViewManager.time_graph_trace(TraceViewHeaderConstant.TASK_TIME_GRAPH_HEAD, result)
        for _tid in tid_set:
            header.append(["thread_name", pid, _tid, "Thread {}".format(_tid)])
        header = TraceViewManager.metadata_event(header)
        header.extend(_trace)
        return json.dumps(header)

    def update_dynamic_info(self: any, dynamic_data: list) -> list:
        """
        update hash value from ge hash data
        :param dynamic_data:
        """
        dynamic_data_list = []
        hash_dict = get_ge_hash_dict(self._project_path)
        for _data in dynamic_data:
            _data = list(_data)
            _data[1] = hash_dict.get(_data[1], _data[1])  # op name
            _data[2] = hash_dict.get(_data[2], _data[2])  # op type
            _data[3] = hash_dict.get(_data[3], _data[3])  # event type
            dynamic_data_list.append(_data)
        return dynamic_data_list

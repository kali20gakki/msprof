#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

import json
import logging
from collections import OrderedDict

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msvp_constant import MsvpConstant
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from msmodel.runtime.runtime_api_view_model import RuntimeApiViewModel
from viewer.memory_copy.memory_copy_viewer import MemoryCopyViewer


class RuntimeApiViewer:

    def __init__(self: any, configs: dict, params: dict) -> None:
        self._configs = configs
        self._params = params
        self._project_path = params.get(StrConstant.PARAM_RESULT_DIR)
        self._model = RuntimeApiViewModel(params)

    @staticmethod
    def _get_runtime_data(timeline_data: list, pid: str) -> list:
        trace_data = []
        for sql_data in timeline_data:
            runtime_name = str(sql_data[0]) if str(sql_data[0]) else Constant.NA
            args = OrderedDict([('Thread Id', sql_data[3])])
            args = MemoryCopyViewer.add_runtime_memcpy_args(sql_data, args)
            trace_data.append(
                (runtime_name, pid,
                 sql_data[3], sql_data[1] / NumberConstant.CONVERSION_TIME,
                 sql_data[2] / NumberConstant.CONVERSION_TIME,
                 args))
        return trace_data

    @staticmethod
    def _get_runtime_result_data(timeline_data: list, pid: str, tid: str) -> list:
        result_data = []
        tid_values = set()
        for sql_data in timeline_data:
            tid_values.add(sql_data[3])

        meta_data = [["process_name", pid, tid, TraceViewHeaderConstant.PROCESS_RUNTIME]]
        meta_data.extend(["thread_name", pid, tid_value, f"Thread {tid_value}"] for tid_value in tid_values)
        meta_data.extend(["thread_sort_index", pid, tid_value, tid_value] for tid_value in tid_values)
        result_data.extend(TraceViewManager.metadata_event(meta_data))
        return result_data

    @staticmethod
    def _summary_reformat(summary_data: list) -> list:
        return [
            (
                data[0], data[1], data[2],
                InfoConfReader().get_host_duration(data[3]),
                data[4],
                InfoConfReader().get_host_duration(data[5]),
                InfoConfReader().get_host_duration(data[6]),
                InfoConfReader().get_host_duration(data[7]),
                data[8], data[9]
            ) for data in summary_data
        ]

    @staticmethod
    def _timeline_reformat(timeline_data: list) -> list:
        return [
            (
                data[0], InfoConfReader().time_from_host_syscnt(data[1]),
                InfoConfReader().get_host_duration(data[2]),
                data[3], data[4], data[5], data[6], data[7]
            ) for data in timeline_data
        ]

    def get_summary_data(self: any) -> tuple:
        """
        get summary data from acl data
        :return: summary data
        """
        if not self._model.init():
            logging.error("Maybe acl data parse failed, please check the data parsing log.")
            return MsvpConstant.MSVP_EMPTY_DATA
        summary_data = self._model.get_summary_data()
        summary_data = self._summary_reformat(summary_data)
        if summary_data:
            return self._configs.get(StrConstant.CONFIG_HEADERS), summary_data, len(summary_data)
        return MsvpConstant.MSVP_EMPTY_DATA

    def get_timeline_data(self: any) -> str:
        with self._model as _model:
            if not _model.check_db() or not _model.check_table():
                return json.dumps(
                    {"status": NumberConstant.ERROR, "info": f"Failed to connect {DBNameConstant.DB_RUNTIME}"})

            timeline_data = _model.get_timeline_data()
            timeline_data = self._timeline_reformat(timeline_data)
            if not timeline_data:
                return json.dumps(
                    {"status": NumberConstant.WARN, "info": f"Unable to get runtime api data."})
            pid = InfoConfReader().get_json_pid_data()
            tid = InfoConfReader().get_json_tid_data()
            result_data = self._get_runtime_result_data(timeline_data, pid, tid)
            trace_data = self._get_runtime_data(timeline_data, pid)
            result_data.extend(
                TraceViewManager.time_graph_trace(TraceViewHeaderConstant.TOP_DOWN_TIME_GRAPH_HEAD, trace_data))
            return json.dumps(result_data)

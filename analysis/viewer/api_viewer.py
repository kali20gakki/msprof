#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

import json
import logging
from collections import OrderedDict

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from msmodel.api.api_data_viewer_model import ApiDataViewModel
from viewer.get_trace_timeline import TraceViewer


class ApiViewer:
    """
    Viewer for api data
    """
    ACL_LEVEL = 'acl'
    HCCL_LEVEL = 'hccl'

    def __init__(self: any, configs: dict, params: dict) -> None:
        self._configs = configs
        self._params = params
        self._project_path = params.get(StrConstant.PARAM_RESULT_DIR)
        self._model = ApiDataViewModel(params)

    @staticmethod
    def _get_api_result_data(timeline_data: list, pid: str, tid: str) -> list:
        result_data = []
        tid_values = set()
        for sql_data in timeline_data:
            tid_values.add(sql_data[3])

        meta_data = [["process_name", pid, tid, TraceViewHeaderConstant.PROCESS_API]]
        meta_data.extend(["thread_name", pid, tid_value, f"Thread {tid_value}"] for tid_value in tid_values)
        meta_data.extend(["thread_sort_index", pid, tid_value, tid_value] for tid_value in tid_values)
        result_data.extend(TraceViewManager.metadata_event(meta_data))
        return result_data

    @staticmethod
    def _get_data_api_name(api_data: list) -> str:
        level = api_data[4]
        if level == ApiViewer.ACL_LEVEL:
            api_name = str(api_data[5]) if str(api_data[5]) else Constant.NA
        elif level == ApiViewer.HCCL_LEVEL:
            api_name = str(api_data[6]) if str(api_data[6]) else Constant.NA
        else:
            api_name = str(api_data[0]) if str(api_data[0]) else Constant.NA
        return api_name

    @staticmethod
    def _get_api_data(timeline_data: list, pid: str) -> list:
        trace_data = []
        for sql_data in timeline_data:
            args = OrderedDict([('Thread Id', sql_data[3])])
            args.setdefault("Mode", sql_data[0])
            args.setdefault("level", sql_data[4])
            args.setdefault("id", sql_data[5])
            args.setdefault("item_id", sql_data[6])
            trace_data.append(
                (ApiViewer._get_data_api_name(sql_data), pid,
                 sql_data[3], sql_data[1] / NumberConstant.CONVERSION_TIME,
                 sql_data[2] / NumberConstant.CONVERSION_TIME,
                 args))
        return trace_data

    def get_timeline_data(self: any) -> str:
        """
        get timeline data from api data
        :return:
        """
        with self._model as _model:
            if not _model.check_db() or not _model.check_table():
                return json.dumps({"status": NumberConstant.ERROR,
                                    "info": f"Failed to connect {DBNameConstant.DB_API_EVENT}"})
            timeline_data = _model.get_timeline_data()
            if not timeline_data:
                return json.dumps(
                    {"status": NumberConstant.WARN, "info": f"Unable to get api data."})
            pid = InfoConfReader().get_json_pid_data()
            tid = InfoConfReader().get_json_tid_data()
            result_data = self._get_api_result_data(timeline_data, pid, tid)
            trace_data = self._get_api_data(timeline_data, pid)
            result_data.extend(
                TraceViewManager.time_graph_trace(TraceViewHeaderConstant.TOP_DOWN_TIME_GRAPH_HEAD, trace_data))
            return json.dumps(result_data)
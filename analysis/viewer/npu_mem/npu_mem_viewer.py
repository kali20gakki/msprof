#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

import json
import logging
from collections import OrderedDict

from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msvp_constant import MsvpConstant
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from msmodel.npu_mem.npu_mem_model import NpuMemModel
from viewer.get_trace_timeline import TraceViewer


class NpuMemViewer:

    NPU_MEM_TYPE_APP = "0"
    NPU_MEM_TYPE_DEVICE = "1"

    def __init__(self: any, configs: dict, params: dict) -> None:
        self._configs = configs
        self._params = params
        self._project_path = params.get(StrConstant.PARAM_RESULT_DIR)
        self._model = NpuMemModel(self._project_path,
                                  DBNameConstant.DB_NPU_MEM,
                                  [DBNameConstant.TABLE_NPU_MEM])
        self._npu_mem_events = {self.NPU_MEM_TYPE_APP: "APP", self.NPU_MEM_TYPE_DEVICE: "Device"}

    def get_summary_data(self: any) -> tuple:
        """
        get summary data from npu mem data
        :return: summary data
        """
        with self._model as _model:
            if not _model.check_db() or not _model.check_table():
                logging.error("Maybe npu mem data parse failed, please check the data parsing log.")
                return MsvpConstant.MSVP_EMPTY_DATA
            summary_data = self._model.get_summary_data()
            if summary_data:
                summary_data = [[self._npu_mem_events.get(datum.event),
                                 datum.ddr, datum.hbm, datum.memory, datum.timestamp]
                                for datum in summary_data]
                return self._configs.get(StrConstant.CONFIG_HEADERS), summary_data, len(summary_data)
            return MsvpConstant.MSVP_EMPTY_DATA

    def get_timeline_data(self: any) -> str:
        with self._model as _model:
            if not _model.check_db() or not _model.check_table():
                return json.dumps(
                    {"status": NumberConstant.ERROR, "info": f"Failed to connect {DBNameConstant.DB_NPU_MEM}"})

            timeline_data = _model.get_timeline_data()
            if not timeline_data:
                return json.dumps(
                    {"status": NumberConstant.ERROR, "info": f"Unable to get npu mem data."})
            pid = InfoConfReader().get_json_pid_data()
            tid = InfoConfReader().get_json_tid_data()
            trace_parser = TraceViewer("NPU_MEM")
            meta_data = [["process_name", pid, tid, trace_parser.scope]]
            _result = TraceViewManager.metadata_event(meta_data)
            column_trace_data = []
            for datum in timeline_data:
                column_trace_data.append(['{}/DDR'.format(self._npu_mem_events.get(datum.event)), datum.timestamp,
                                          pid, tid, OrderedDict([("KB", datum.ddr)])])
                column_trace_data.append(['{}/HBM'.format(self._npu_mem_events.get(datum.event)), datum.timestamp,
                                          pid, tid, OrderedDict([("KB", datum.hbm)])])
                column_trace_data.append(['{}/Memory'.format(self._npu_mem_events.get(datum.event)), datum.timestamp,
                                          pid, tid, OrderedDict([("KB", datum.memory)])])
            _result += \
                TraceViewManager.column_graph_trace(TraceViewHeaderConstant.COLUMN_GRAPH_HEAD_LEAST,
                                                    column_trace_data)
            if _result:
                return trace_parser.format_trace_events(_result)
            return json.dumps({"status": NumberConstant.ERROR, "info": "No data is collected."})

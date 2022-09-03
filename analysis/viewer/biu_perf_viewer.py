#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.

import json
from collections import OrderedDict

from msmodel.biu_perf.biu_perf_model import BiuPerfModel
from common_func.trace_view_manager import TraceViewManager
from common_func.db_name_constant import DBNameConstant
from common_func.trace_view_header_constant import TraceViewHeaderConstant


class BiuPerfViewer:
    """
    biu perf viewer
    """

    def __init__(self: any, project_path: str) -> None:
        self._project_path = project_path
        self._model = BiuPerfModel(self._project_path,
                                   [DBNameConstant.TABLE_BIU_FLOW,
                                    DBNameConstant.TABLE_BIU_CYCLES])

    def get_timeline(self: any) -> str:
        meta_timeline = self.get_meta_timeline()
        biu_flow_timeline = self.get_biu_flow_timeline()
        biu_cycles_timeline = self.get_biu_cycles_timeline()
        return json.dumps(meta_timeline + biu_flow_timeline + biu_cycles_timeline)

    def get_meta_timeline(self: any) -> list:
        meta_timeline = []
        with self._model as _model:
            meta_timeline.extend(TraceViewManager.metadata_event(self._model.get_biu_flow_process()))
            meta_timeline.extend(TraceViewManager.metadata_event(self._model.get_biu_flow_thread()))
            meta_timeline.extend(TraceViewManager.metadata_event(self._model.get_biu_cycles_process()))
            meta_timeline.extend(TraceViewManager.metadata_event(self._model.get_biu_cycles_thread()))
        return meta_timeline

    def get_biu_flow_timeline(self: any) -> list:
        trace_data = []
        with self._model as _model:
            biu_flow_data = self._model.get_biu_flow_data()
        for biu_flow_datum in biu_flow_data:
            trace_datum = []
            # 0 flow_type; 1 interval_start; 2 pid; 3 tid; 4 flow
            trace_datum.append(biu_flow_datum[0])
            trace_datum.append(biu_flow_datum[1])
            trace_datum.append(biu_flow_datum[2])
            trace_datum.append(biu_flow_datum[3])
            trace_datum.append(OrderedDict([("flow", biu_flow_datum[4])]))
            trace_data.append(trace_datum)
        return TraceViewManager.column_graph_trace(TraceViewHeaderConstant.COLUMN_GRAPH_HEAD_LEAST, trace_data)

    def get_biu_cycles_timeline(self: any) -> list:
        trace_data = []
        with self._model as _model:
            biu_cycles_data = self._model.get_biu_cycles_data()
        for biu_cycles_datum in biu_cycles_data:
            trace_datum = []
            # 0 pid; 1 tid; 2 interval_start; 3 duration; 4 cycle_num; 5 ratio
            trace_datum.append("")
            trace_datum.append(biu_cycles_datum[0])
            trace_datum.append(biu_cycles_datum[1])
            trace_datum.append(biu_cycles_datum[2])
            trace_datum.append(biu_cycles_datum[3])
            trace_datum.append(OrderedDict([("cycle_num", biu_cycles_datum[4]),
                                            ("ratio", biu_cycles_datum[5])]))
            trace_data.append(trace_datum)
        return TraceViewManager.time_graph_trace(TraceViewHeaderConstant.TOP_DOWN_TIME_GRAPH_HEAD, trace_data)

#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2024. All rights reserved.

import itertools
from abc import ABC
from collections import defaultdict

from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.trace_view_manager import TraceViewManager
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from msmodel.hardware.qos_viewer_model import QosViewModel
from viewer.interface.base_viewer import BaseViewer


class QosViewer(BaseViewer, ABC):
    """
    class for get qos data
    """

    def __init__(self: any, configs: dict, params: dict) -> None:
        super().__init__(configs, params)
        self.pid = 0
        self.model_list = {
            "qos": QosViewModel
        }

    def get_trace_timeline(self: any, datas: list) -> list:
        """
        format data to standard timeline format
        :return: list
        """
        results, legends = self._fill_qos_data(datas)
        tmp_timestamp_index = 0
        tmp_bandwidth_index = 1
        _trace = []
        if legends and results:
            pid = InfoConfReader().get_json_pid_data()
            tid = InfoConfReader().get_json_tid_data()
            meta_data = [["process_name", pid, tid, "QoS"]]
            for key in list(results.keys()):
                column_trace_data = [
                    [key, item[tmp_timestamp_index], pid, tid, {legends.get(key, 0): item[tmp_bandwidth_index]}]
                    for item in results[key]
                ]
                _trace += \
                    TraceViewManager.column_graph_trace(TraceViewHeaderConstant.COLUMN_GRAPH_HEAD_LEAST,
                                                        column_trace_data)
            result = TraceViewManager.metadata_event(meta_data)
            result.extend(_trace)
            return result
        return []

    def _fill_qos_data(self, qos_data) -> (dict, dict):
        timestamp_index = 0
        bandwidth_index = 1
        mpam_id_index = 2
        event_type_index = 3
        qos_events = {"read": "Read(MB/s)", "write": "Write(MB/s)"}
        mpam_id_list = []
        timeline_results = defaultdict(list)
        timeline_legends = dict()
        qos_description = self._get_qos_description()
        for data in qos_data:
            local_time = data[timestamp_index] / NumberConstant.NS_TO_US
            key = "QoS {}/{}".format(qos_description.get(data[mpam_id_index], data[mpam_id_index]),
                                     data[event_type_index])
            timeline_results[key].append([local_time, data[bandwidth_index]])
            mpam_id = data[mpam_id_index]
            if mpam_id not in mpam_id_list:
                mpam_id_list.append(mpam_id)
        for _mpam_id, _direct in itertools.product(mpam_id_list, qos_events.keys()):
            key = "QoS {}/{}".format(qos_description.get(_mpam_id, _mpam_id), _direct)
            timeline_legends[key] = qos_events.get(_direct, 0)
        return timeline_results, timeline_legends

    def _get_qos_description(self):
        model = self.get_model_instance()
        model.table_list = [DBNameConstant.TABLE_QOS_INFO, DBNameConstant.TABLE_QOS_ORIGIN]
        if not model or not model.check_table():
            return {}
        qos_info = model.get_qos_info()
        return dict(qos_info)

# coding:utf-8
# -*- coding=utf8 -*-
"""
This script is used to provide acc_pmu reports
Copyright Huawei Technologies Co., Ltd. 2021. All rights reserved.
"""

from abc import ABC

from common_func.info_conf_reader import InfoConfReader
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from model.stars.acc_pmu_model import AccPmuModel
from viewer.interface.base_viewer import BaseViewer


class AccPmuViewer(BaseViewer, ABC):
    """
    class for get acc_pmu data
    """

    DATA_TYPE = 'data_type'
    SAMPLE_BASED = 'sample_based'

    def __init__(self: any, configs: dict, params: dict) -> None:
        super().__init__(configs, params)
        self.pid = 0
        self.model_list = {
            'acc_pmu_summary': AccPmuModel,
        }

    def get_timeline_header(self: any) -> list:
        """
        to get chrome trace json header
        """
        acc_header = [["process_name", self.pid,
                       InfoConfReader().get_json_tid_data(), self.params.get(self.DATA_TYPE)]]
        return acc_header

    def get_trace_timeline(self: any, datas: list) -> list:
        """
        to format data to chrome trace json
        """
        if not datas:
            return []
        result = []
        for data in datas:
            result.append(["acc_id " + str(data[2]), data[9],
                           {'read_bandwidth': data[5], 'write_band_width': data[6],
                            'read_ost': data[7], 'write_ost': data[8]}])
        pid = self.pid
        for item in result:
            item[2:2] = [pid, InfoConfReader().get_json_tid_data()]
        _trace = TraceViewManager.column_graph_trace(TraceViewHeaderConstant.COLUMN_GRAPH_HEAD_LEAST, result)
        result = TraceViewManager.metadata_event(self.get_timeline_header())
        result.extend(_trace)
        self.pid += 1
        return result

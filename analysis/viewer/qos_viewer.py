# -------------------------------------------------------------------------
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
# This file is part of the MindStudio project.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#    http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------
from abc import ABC
import logging

from common_func.info_conf_reader import InfoConfReader
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
        self.model_list = {
            "qos": QosViewModel
        }

    def get_trace_timeline(self: any, datas: list) -> list:
        """
        format data to standard timeline format
        :return: list
        """
        qos_events = InfoConfReader().get_qos_events()
        if not qos_events:
            logging.error("qosEvents is not invalid in sample.json.")
            return []
        timestamp_index = 0
        bandwidth_index = 1
        column_trace_data = []
        key_list = ["QoS {}".format(i) for i in qos_events.split(",")]
        pid = InfoConfReader().get_json_pid_data()
        tid = InfoConfReader().get_json_tid_data()
        for data in datas:
            local_time = InfoConfReader().trans_into_local_time(raw_timestamp=data[timestamp_index])
            for key, value in zip(key_list, data[bandwidth_index:]):
                column_trace_data.append([key, local_time, pid, tid, {"value": value}])
        meta_data = [["process_name", pid, tid, "QoS"]]
        result = TraceViewManager.metadata_event(meta_data)
        result.extend(TraceViewManager.column_graph_trace(TraceViewHeaderConstant.COLUMN_GRAPH_HEAD_LEAST,
                                                          column_trace_data))
        return result

#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2024. All rights reserved.

from abc import ABC

from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.msvp_common import format_high_precision_for_csv
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from msmodel.hardware.ub_model import UBViewModel
from viewer.interface.base_viewer import BaseViewer


class UBViewer(BaseViewer, ABC):
    """
    class for get ub data
    """

    def __init__(self: any, configs: dict, params: dict) -> None:
        super().__init__(configs, params)
        self.model_list = {
            "ub": UBViewModel
        }
        self.pid = InfoConfReader().get_json_pid_data()
        self.tid = InfoConfReader().get_json_tid_data()

    def get_column_trace_data(self, datas: list) -> list:
        column_trace_data = []
        for data in datas:
            local_time = InfoConfReader().trans_into_local_time(
                    raw_timestamp=InfoConfReader().get_host_time_by_sampling_timestamp(data.time_stamp),
                    use_us=True)
            port = "Port" + f"{data.port_id:03d}"
            column_trace_data.append(
                ["UNIC-" + port, local_time, self.pid, self.tid, {"bandwidth_rx(MB/s)": data.rx_port_band_width,
                "bandwidth_tx(MB/s)": data.tx_port_band_width}])
            column_trace_data.append(
                ["UDMA-" + port, local_time, self.pid, self.tid, {"bandwidth_rx(MB/s)": data.udma_rx_bind,
                "bandwidth_tx(MB/s)": data.udma_tx_bind}])
        return column_trace_data

    def get_trace_timeline(self: any, datas: list) -> list:
        """
        format data to standard timeline format
        :return: list
        """
        column_trace_data = self.get_column_trace_data(datas)
        meta_data = [["process_name", self.pid, self.tid, "UB"]]
        result = TraceViewManager.metadata_event(meta_data)
        result.extend(TraceViewManager.column_graph_trace(TraceViewHeaderConstant.COLUMN_GRAPH_HEAD_LEAST,
                                                          column_trace_data))
        return result

    def get_summary_data(self: any) -> tuple:
        """
        to get summary data
        """
        summary_data = self.get_data_from_db()
        formatted_data = self.format_ub_summary_data(summary_data)
        return self.configs.get(StrConstant.CONFIG_HEADERS), formatted_data, len(formatted_data)

    def format_ub_summary_data(self, summary_data: list) -> list:
        return [
            (
                data.port_id,
                format_high_precision_for_csv(
                    InfoConfReader().trans_into_local_time(
                        raw_timestamp=InfoConfReader().get_host_time_by_sampling_timestamp(data.time_stamp),
                        use_us=True)
                ),
                data.udma_rx_bind, data.udma_tx_bind, data.rx_port_band_width, data.tx_port_band_width, data.rx_packet_rate,
                data.rx_bytes, data.rx_packets, data.rx_errors, data.rx_dropped, data.tx_packet_rate,
                data.tx_bytes, data.tx_packets, data.tx_errors, data.tx_dropped
            ) for data in summary_data
        ]

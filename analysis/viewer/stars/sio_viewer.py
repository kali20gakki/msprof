#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.

from abc import ABC

from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from msmodel.stars.sio_model import SioModel
from viewer.interface.base_viewer import BaseViewer


class SioViewer(BaseViewer, ABC):
    """
    class for get sio data
    """

    DATA_TYPE = 'data_type'
    # 标号为在sio.db中的列索引
    ACC_ID = 0
    REQ_RX = 1
    RSP_RX = 2
    SNP_RX = 3
    DAT_RX = 4
    REQ_TX = 5
    RSP_TX = 6
    SNP_TX = 7
    DAT_TX = 8
    TIME_STAMP = 9

    def __init__(self: any, configs: dict, params: dict) -> None:
        super().__init__(configs, params)
        self.pid = 0
        self.model_list = {
            'sio': SioModel
        }

    def get_trace_timeline(self: any, datas: list) -> list:
        """
        format data to standard timeline format
        :return: list
        """
        if not datas:
            return []
        bandwidth_name = ["req_rx", "rsp_rx", "snp_rx", "dat_rx", "req_tx", "rsp_tx", "snp_tx", "dat_tx"]
        result = []
        pid = self.pid
        tid = InfoConfReader().get_json_tid_data()
        value_key = "value"
        acc_id_key = "acc_id"
        timestamp_dict = {}
        for data in datas:
            acc_id = data[self.ACC_ID]
            timestamp = data[self.TIME_STAMP]
            if acc_id in timestamp_dict:
                data_size_list = data[self.REQ_RX: self.TIME_STAMP]
                bandwidth_data = [data_size / (NumberConstant.BYTES_TO_KB ** 2) / ((
                                 timestamp - timestamp_dict[acc_id]) / NumberConstant.NANO_SECOND)
                                  for data_size in data_size_list]
                local_time = InfoConfReader().trans_into_local_time(raw_timestamp=data[self.TIME_STAMP], use_us=True)
                for index, bandwidth in enumerate(bandwidth_data):
                    result.append([bandwidth_name[index], local_time, pid, tid,
                                   {value_key: bandwidth, acc_id_key: acc_id}])
            timestamp_dict[acc_id] = timestamp
        _trace = TraceViewManager.column_graph_trace(TraceViewHeaderConstant.COLUMN_GRAPH_HEAD_LEAST, result)
        result = TraceViewManager.metadata_event([["process_name", pid, tid, self.params.get(self.DATA_TYPE)]])
        result.extend(_trace)
        self.pid += 1
        return result

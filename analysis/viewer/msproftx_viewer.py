#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

import logging
from collections import OrderedDict

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msvp_constant import MsvpConstant
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from common_func.utils import Utils
from msmodel.msproftx.msproftx_model import MsprofTxModel, MsprofTxExModel
from viewer.get_trace_timeline import TraceViewer


class MsprofTxViewer:
    """
    class for get msproftx data
    """

    def __init__(self: any, configs: dict, params: dict) -> None:
        self.configs = configs
        self.params = params

    @staticmethod
    def get_time_timeline_header(msproftx_datas: tuple) -> list:
        """
        to get sequence chrome trace json header
        :return: header of trace data list
        """
        pid_values = set()
        tid_values = set()
        for msproftx_data in msproftx_datas:
            pid_values.add(msproftx_data.pid)
            tid_values.add(msproftx_data.tid)
        meta_data = Utils.generator_to_list(["process_name", pid_value,
                                             InfoConfReader().get_json_tid_data(),
                                             TraceViewHeaderConstant.PROCESS_MSPROFTX]
                                            for pid_value in pid_values)
        meta_data.extend(Utils.generator_to_list(["thread_name", pid_value, tid_value,
                                                  "Thread {}".format(tid_value)] for tid_value in tid_values
                                                 for pid_value in pid_values))
        meta_data.extend(Utils.generator_to_list(["thread_sort_index", pid_value, tid_value,
                                                  tid_value] for tid_value in tid_values
                                                 for pid_value in pid_values))

        return meta_data

    @staticmethod
    def format_tx_timeline_data(msproftx_data: tuple) -> list:
        """
        to format data to chrome trace json
        :return: timeline_trace list
        """
        trace_data = []
        for top_down_data in msproftx_data:
            trace_data_args = OrderedDict([
                ("Category", str(top_down_data.category)),
                ("Payload_type", top_down_data.payload_type),
                ("Payload_value", top_down_data.payload_value),
                ("Message_type", top_down_data.message_type),
                ("event_type", top_down_data.event_type)
            ])
            trace_data_msproftx = [
                top_down_data.message, top_down_data.pid, top_down_data.tid,
                InfoConfReader().trans_into_local_time(
                    InfoConfReader().time_from_host_syscnt(top_down_data.start_time, NumberConstant.MICRO_SECOND),
                    use_us=True),
                InfoConfReader().get_host_duration(top_down_data.dur_time,
                                                   NumberConstant.MICRO_SECOND),
                trace_data_args
            ]
            trace_data.append(trace_data_msproftx)
        return trace_data

    @staticmethod
    def format_tx_ex_timeline_data(msproftx_ex_data: tuple) -> list:
        """
        to format msprof ex data to chrome trace json
        :param msproftx_ex_data: msprof ex data
        :return: timeline_trace list
        """
        trace_data = []
        for data in msproftx_ex_data:
            trace_data_args = OrderedDict([
                ('mark_id', data.mark_id),
                ("event_type", data.event_type)
            ])
            trace_data_msproftx_ex = [
                data.message, data.pid, data.tid,
                InfoConfReader().trans_into_local_time(
                    InfoConfReader().time_from_host_syscnt(data.start_time, NumberConstant.MICRO_SECOND),
                    use_us=True),
                InfoConfReader().get_host_duration(data.dur_time, NumberConstant.MICRO_SECOND),
                trace_data_args
            ]
            trace_data.append(trace_data_msproftx_ex)
        return trace_data

    @staticmethod
    def format_tx_summary_data(summary_data: list) -> list:
        return [
            (
                data[0], data[1], data[2], data[3], data[4], data[5],
                InfoConfReader().trans_into_local_time(
                    InfoConfReader().time_from_host_syscnt(data[6], NumberConstant.MICRO_SECOND), use_us=True),
                InfoConfReader().trans_into_local_time(
                    InfoConfReader().time_from_host_syscnt(data[7], NumberConstant.MICRO_SECOND), use_us=True),
                data[8], data[9]
            ) for data in summary_data
        ]

    @staticmethod
    def format_tx_ex_summary_data(summary_data: list) -> list:
        return [
            (data[0], data[1], Constant.NA, data[2], Constant.NA, Constant.NA,
             InfoConfReader().trans_into_local_time(
                 InfoConfReader().time_from_host_syscnt(data[3], NumberConstant.MICRO_SECOND), use_us=True),
             InfoConfReader().trans_into_local_time(
                 InfoConfReader().time_from_host_syscnt(data[4], NumberConstant.MICRO_SECOND), use_us=True),
             Constant.NA, data[5]
             ) for data in summary_data
        ]

    def get_summary_data(self: any) -> tuple:
        """
        to get summary data
        :return:summary data
        """
        with MsprofTxModel(self.params.get('project'),
                           DBNameConstant.DB_MSPROFTX,
                           [DBNameConstant.TABLE_MSPROFTX]) as tx_model:
            msproftx_data = tx_model.get_summary_data()
        with MsprofTxExModel(self.params.get('project')) as msproftx_ex_model:
            msproftx_ex_data = msproftx_ex_model.get_summary_data()
        if not msproftx_data and not msproftx_ex_data:
            return MsvpConstant.MSVP_EMPTY_DATA
        msproftx_data = self.format_tx_summary_data(msproftx_data)
        msproftx_ex_data = self.format_tx_ex_summary_data(msproftx_ex_data)
        summary_data = msproftx_data + msproftx_ex_data
        summary_data.sort(key=lambda x: x[6])
        return self.configs.get('headers'), summary_data, len(summary_data)

    def get_timeline_data(self: any) -> any:
        """
        to get timeline data
        :return:timeline data
        """
        TraceViewHeaderConstant.update_layer_info_map(InfoConfReader().get_json_pid_name())
        with MsprofTxModel(self.params.get('project'),
                           DBNameConstant.DB_MSPROFTX,
                           [DBNameConstant.TABLE_MSPROFTX]) as tx_model:
            msproftx_data = tx_model.get_timeline_data()
        with MsprofTxExModel(self.params.get('project')) as msproftx_ex_model:
            msproftx_ex_data = msproftx_ex_model.get_timeline_data()
        if not msproftx_data and not msproftx_ex_data:
            return '[]'
        tx_trace_data = self.format_tx_timeline_data(msproftx_data)
        msproftx_ex_trace_data = self.format_tx_ex_timeline_data(msproftx_ex_data)
        _trace = TraceViewManager.time_graph_trace(TraceViewHeaderConstant.TOP_DOWN_TIME_GRAPH_HEAD,
                                                   tx_trace_data + msproftx_ex_trace_data)
        result = TraceViewManager.metadata_event(self.get_time_timeline_header(msproftx_data + msproftx_ex_data))
        result.extend(_trace)
        return result


# coding=utf-8
"""
This script is used to provide msproftx data reports
Copyright Huawei Technologies Co., Ltd. 2021. All rights reserved.
"""
import logging
import sqlite3
from collections import OrderedDict

from common_func.constant import Constant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msvp_constant import MsvpConstant
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from common_func.utils import Utils
from model.msproftx.msproftx_model import MsprofTxModel
from viewer.get_trace_timeline import TraceViewer


class MsprofTxViewer:
    """
    class for get msproftx data
    """

    def __init__(self: any, configs: dict, params: dict) -> None:
        self.configs = configs
        self.params = params
        self.model = None

    def get_summary_data(self: any) -> tuple:
        """
        to get summary data
        :return:summary data
        """
        self.init_model()
        try:
            msproftx_data = self.model.get_all_data(self.configs.get('table'))
            return self.configs.get('headers'), msproftx_data, len(msproftx_data)
        except (ValueError, IOError, TypeError) as error:
            logging.error(error, exc_info=Constant.TRACE_BACK_SWITCH)
            return MsvpConstant.MSVP_EMPTY_DATA
        finally:
            self.model.finalize()

    def get_timeline_data(self: any) -> str:
        """
        to get timeline data
        :return:timeline data
        """
        self.init_model()
        msproftx_data = self.model.get_timeline_data()
        try:
            trace_data = self.format_data(msproftx_data)
        except (ValueError, TypeError) as error:
            logging.error(error, exc_info=Constant.TRACE_BACK_SWITCH)
            return '[]'
        finally:
            self.model.finalize()
        _trace = TraceViewManager.time_graph_trace(TraceViewHeaderConstant.TOP_DOWN_TIME_GRAPH_HEAD, trace_data)
        result = TraceViewManager.metadata_event(self.get_time_timeline_header(msproftx_data))
        result.extend(_trace)
        return TraceViewer("MsprofTxViewer").format_trace_events(result)


    @staticmethod
    def get_time_timeline_header(data: tuple) -> list:
        """
        to get sequence chrome trace json header
        :return: header of trace data list
        """
        pid_values = set()
        tid_values = set()
        for msproftx_data in data:
            pid_values.add(msproftx_data[1])
            tid_values.add(msproftx_data[2])
        meta_data = Utils.generator_to_list(["process_name", pid_value,
                                             InfoConfReader().get_json_tid_data(),
                                             TraceViewHeaderConstant.PEOCESS_MSPROFTX]
                                            for pid_value in pid_values)
        meta_data.extend(Utils.generator_to_list(["thread_name", pid_value, tid_value,
                                                  "Thread {}".format(tid_value)] for tid_value in tid_values
                                                 for pid_value in pid_values))
        meta_data.extend(Utils.generator_to_list(["thread_sort_index", pid_value, tid_value,
                                                  tid_value] for tid_value in tid_values
                                                 for pid_value in pid_values))

        return meta_data

    @staticmethod
    def format_data(msproftx_data: tuple) -> list:
        """
        to format data to chrome trace json
        :return: timeline_trace list
        """
        trace_data = []
        for top_down_data in msproftx_data:
            trace_data_args = OrderedDict([
                ("Category", str(top_down_data[0])),
                ("Payload_type", top_down_data[5]),
                ("Payload_value", top_down_data[6]),
                ("Message_type", top_down_data[7]),
                ("event_type", top_down_data[9])
            ])
            trace_data_msproftx = [top_down_data[8], top_down_data[1], top_down_data[2],
                                   int(top_down_data[3]) / NumberConstant.CONVERSION_TIME,
                                   int(top_down_data[4]) / NumberConstant.CONVERSION_TIME,
                                   trace_data_args]
            trace_data.append(trace_data_msproftx)
        return trace_data

    def init_model(self: any) -> None:
        """
        init model to get data from db
        """
        self.model = MsprofTxModel(self.params.get('project'), self.configs.get('db'), [self.configs.get('table')])
        self.model.init()

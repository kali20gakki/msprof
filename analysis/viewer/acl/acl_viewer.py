#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

import json
import logging

from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msvp_constant import MsvpConstant
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from common_func.utils import Utils
from msmodel.acl.acl_model import AclModel
from viewer.get_trace_timeline import _format_single_data
from viewer.get_trace_timeline import _get_acl_data


class AclViewer:
    """
    Viewer for showing acl data
    """

    def __init__(self: any, configs: dict, params: dict) -> None:
        self._configs = configs
        self._params = params
        self._project_path = params.get(StrConstant.PARAM_RESULT_DIR)
        self._model = AclModel(params)

    @staticmethod
    def _summary_reformat(summary_data: list) -> list:
        return [
            (
                data[0], data[1], InfoConfReader().time_from_host_syscnt(data[2]),
                InfoConfReader().get_host_duration(data[3], NumberConstant.MICRO_SECOND),
                data[4], data[5]
            ) for data in summary_data
        ]

    @staticmethod
    def _timeline_reformat(timeline_data: list) -> list:
        return [
            (
                data[0], InfoConfReader().time_from_host_syscnt(data[1]),
                InfoConfReader().get_host_duration(data[2]),
                data[3], data[4], data[5]
            ) for data in timeline_data
        ]

    @staticmethod
    def _acl_statistic_reformat(acl_data: list) -> list:
        return [
            (
                data[0], data[1], data[2],
                InfoConfReader().get_host_duration(data[3], NumberConstant.MICRO_SECOND),
                data[4],
                InfoConfReader().get_host_duration(data[5], NumberConstant.MICRO_SECOND),
                InfoConfReader().get_host_duration(data[6], NumberConstant.MICRO_SECOND),
                InfoConfReader().get_host_duration(data[7], NumberConstant.MICRO_SECOND),
                data[8], data[9]
            ) for data in acl_data
        ]

    def get_summary_data(self: any) -> tuple:
        """
        get summary data from acl data
        :return: summary data
        """
        if not self._model.init():
            logging.error("Maybe acl data parse failed, please check the data parsing log.")
            return MsvpConstant.MSVP_EMPTY_DATA
        summary_data = self._model.get_summary_data()
        summary_data = self._summary_reformat(summary_data)
        if summary_data:
            return self._configs.get(StrConstant.CONFIG_HEADERS), summary_data, len(summary_data)
        return MsvpConstant.MSVP_EMPTY_DATA

    def get_timeline_data(self: any) -> str:
        """
        get timeline data from acl data
        :return:
        """
        with self._model as _model:
            if not _model.check_db() or not _model.check_table():
                logging.error("Maybe Acl data parse failed, please check the data parsing log.")
                return json.dumps({"status": NumberConstant.ERROR,
                                   "info": "No acl data found, maybe the switch of acl is not on."})
            timeline_data = _model.get_timeline_data()
            timeline_data = self._timeline_reformat(timeline_data)
        if not timeline_data:
            return json.dumps(
                {"status": NumberConstant.ERROR, "info": f"Failed to connect {DBNameConstant.DB_ACL_MODULE}."})
        top_down_datas = Utils.generator_to_list((TraceViewHeaderConstant.PROCESS_ACL,) + acl_iter_data
                                                 for acl_iter_data in timeline_data)
        trace_data = _format_single_data(top_down_datas)
        result_data = _get_acl_data(timeline_data)
        result_data.extend(TraceViewManager.time_graph_trace(
            TraceViewHeaderConstant.TOP_DOWN_TIME_GRAPH_HEAD, trace_data))
        return json.dumps(result_data)

    def get_acl_statistic_data(self) -> tuple:
        """
        get acl statistic data
        """
        with self._model as _model:
            if not _model.check_db() or not _model.check_table():
                logging.error("Maybe Acl data parse failed, please check the data parsing log.")
                return MsvpConstant.MSVP_EMPTY_DATA
            acl_data = _model.get_acl_statistic_data()
            acl_data = self._acl_statistic_reformat(acl_data)
        if not acl_data:
            return MsvpConstant.MSVP_EMPTY_DATA
        return self._configs.get(StrConstant.CONFIG_HEADERS), acl_data, len(acl_data)

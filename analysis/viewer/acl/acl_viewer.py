#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

import json
import logging

from common_func.db_name_constant import DBNameConstant
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

    def get_summary_data(self: any) -> tuple:
        """
        get summary data from acl data
        :return: summary data
        """
        if not self._model.init():
            logging.error("Maybe acl data parse failed, please check the data parsing log.")
            return MsvpConstant.MSVP_EMPTY_DATA
        summary_data = self._model.get_summary_data()
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
        if not acl_data:
            return MsvpConstant.MSVP_EMPTY_DATA
        return self._configs.get(StrConstant.CONFIG_HEADERS), acl_data, len(acl_data)

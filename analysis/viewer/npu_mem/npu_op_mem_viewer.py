#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

import json
import logging

from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msvp_constant import MsvpConstant
from msmodel.npu_mem.npu_op_mem_model import NpuOpMemModel


class NpuOpMemViewer:
    def __init__(self: any, configs: dict, params: dict, table_name: str) -> None:
        self._configs = configs
        self._params = params
        self._project_path = params.get(StrConstant.PARAM_RESULT_DIR)
        self._table_name = table_name
        self._model = NpuOpMemModel(self._project_path, [table_name])

    def get_summary_data(self: any) -> tuple:
        """
        get summary data from npu mem data
        :return: summary data
        """

        if not self._model.check_db() or not self._model.check_table():
            logging.error("Maybe npu op mem data parse failed, please check the data parsing log.")
            return MsvpConstant.MSVP_EMPTY_DATA
        summary_data = self._model.get_summary_data(self._table_name)
        if summary_data:
            if self._table_name == DBNameConstant.TABLE_NPU_OP_MEM:
                summary_data = [[datum.name,
                                 datum.size / NumberConstant.KILOBYTE,
                                 int(datum.allocation_time) / NumberConstant.MILLI_SECOND,
                                 int(datum.release_time) / NumberConstant.MILLI_SECOND \
                                     if datum.release_time != NumberConstant.EXCEPTION else "",
                                 int(datum.duration) / NumberConstant.MILLI_SECOND \
                                     if datum.duration != NumberConstant.EXCEPTION else "",
                                 datum.allocation_total_allocated / NumberConstant.KILOBYTE / NumberConstant.KILOBYTE,
                                 datum.allocation_total_reserved / NumberConstant.KILOBYTE / NumberConstant.KILOBYTE,
                                 datum.release_total_allocated / NumberConstant.KILOBYTE / NumberConstant.KILOBYTE \
                                     if datum.release_total_allocated != NumberConstant.EXCEPTION else "",
                                 datum.release_total_reserved / NumberConstant.KILOBYTE / NumberConstant.KILOBYTE \
                                     if datum.release_total_reserved != NumberConstant.EXCEPTION else "",
                                 datum.device_type]
                                for datum in summary_data]
            elif self._table_name == DBNameConstant.TABLE_NPU_OP_MEM_REC:
                summary_data = [[datum.component,
                                 int(datum.timestamp) / NumberConstant.MILLI_SECOND,
                                 datum.total_allocate_memory / NumberConstant.KILOBYTE / NumberConstant.KILOBYTE,
                                 datum.total_reserve_memory / NumberConstant.KILOBYTE / NumberConstant.KILOBYTE,
                                 datum.device_type]
                                for datum in summary_data]
        return self._configs.get(StrConstant.CONFIG_HEADERS), summary_data, len(summary_data)
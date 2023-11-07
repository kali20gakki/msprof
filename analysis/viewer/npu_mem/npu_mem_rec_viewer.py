#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

import logging

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msvp_constant import MsvpConstant
from msmodel.npu_mem.npu_ai_stack_mem_model import NpuAiStackMemModel
from profiling_bean.prof_enum.data_tag import ModuleName


class NpuMemRecViewer:
    def __init__(self: any, configs: dict, params: dict) -> None:
        self._configs = configs
        self._params = params
        self._project_path = params.get(StrConstant.PARAM_RESULT_DIR)
        self._data = []
        self._model_list = [
            {'table': DBNameConstant.TABLE_NPU_OP_MEM_REC,
             'model': NpuAiStackMemModel(self._project_path,
                                         DBNameConstant.DB_MEMORY_OP,
                                         [DBNameConstant.TABLE_NPU_OP_MEM_REC])},
            {'table': DBNameConstant.TABLE_NPU_MODULE_MEM,
             'model': NpuAiStackMemModel(self._project_path,
                                         DBNameConstant.DB_NPU_MODULE_MEM,
                                         [DBNameConstant.TABLE_NPU_MODULE_MEM])}
        ]

    def get_summary_data(self: any) -> tuple:
        """
        get summary data from npu mem data
        :return: summary data
        """
        for item in self._model_list:
            if not item.get('model').check_db() or not item.get('model').check_table():
                logging.warning("%s data not found, maybe it failed to parse, please check the data parsing log.",
                                item.get('table'))
                continue
            origin_summary_data = item.get('model').get_table_data(item.get('table'))
            if not origin_summary_data:
                logging.error("get %s summary data failed in npu memory record viewer, please check.",
                              item.get('table'))
                return MsvpConstant.MSVP_EMPTY_DATA
            if item.get('table') == DBNameConstant.TABLE_NPU_OP_MEM_REC:
                for datum in origin_summary_data:
                    self._data.append([datum.component,
                                       InfoConfReader().trans_into_local_time(
                                           InfoConfReader().time_from_host_syscnt(int(datum.timestamp),
                                                                                  NumberConstant.MICRO_SECOND)),
                                       datum.total_allocate_memory / NumberConstant.KILOBYTE,
                                       datum.total_reserve_memory / NumberConstant.KILOBYTE,
                                       datum.device_type])
            elif item.get('table') == DBNameConstant.TABLE_NPU_MODULE_MEM:
                for datum in origin_summary_data:
                    if datum.module_id == ModuleName.MAX_MOUDLE_ID.value:
                        logging.error("Invalid module id, please check.")
                        return MsvpConstant.MSVP_EMPTY_DATA
                    self._data.append([ModuleName(datum.module_id).name,
                                       InfoConfReader().trans_into_local_time(
                                           InfoConfReader().time_from_host_syscnt(int(datum.syscnt),
                                                                                  NumberConstant.MICRO_SECOND)),
                                       Constant.NA,
                                       datum.total_size,
                                       datum.device_type
                                       ])
            else:
                logging.error("Invalid table name in npu memory record viewer, please check.")
                return MsvpConstant.MSVP_EMPTY_DATA
        if not self._data:
            return MsvpConstant.MSVP_EMPTY_DATA
        return self._configs.get(StrConstant.CONFIG_HEADERS), self._data, len(self._data)

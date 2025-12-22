#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.path_manager import PathManager
from msmodel.interface.view_model import ViewModel
from common_func.trace_view_header_constant import TraceViewHeaderConstant


class AicVoltageViewerModel(ViewModel):
    """
    class for query voltage.db table AicVoltage
    """

    PROCESSOR_NAME = TraceViewHeaderConstant.PROCESS_AI_CORE_VOLTAGE

    def __init__(self, params: dict) -> None:
        self._result_dir = params.get(StrConstant.PARAM_RESULT_DIR)
        self._iter_range = params.get(StrConstant.PARAM_ITER_ID)
        super().__init__(self._result_dir, DBNameConstant.DB_VOLTAGE,
                         [DBNameConstant.TABLE_AIC_VOLTAGE])

    def get_data(self) -> list:
        """
        query ai core voltage data from voltage.db
        """
        if not DBManager.check_tables_in_db(
                PathManager.get_db_path(self._result_dir, self.db_name), DBNameConstant.TABLE_AIC_VOLTAGE):
            return []
        sql = "select syscnt, voltage from {}".format(DBNameConstant.TABLE_AIC_VOLTAGE)
        return DBManager.fetch_all_data(self.cur, sql)


class BusVoltageViewerModel(ViewModel):
    """
    class for query voltage.db table BusVoltage
    """

    PROCESSOR_NAME = TraceViewHeaderConstant.PROCESS_BUS_VOLTAGE

    def __init__(self, params: dict) -> None:
        self._result_dir = params.get(StrConstant.PARAM_RESULT_DIR)
        self._iter_range = params.get(StrConstant.PARAM_ITER_ID)
        super().__init__(self._result_dir, DBNameConstant.DB_VOLTAGE,
                         [DBNameConstant.TABLE_BUS_VOLTAGE])

    def get_data(self) -> list:
        """
        query bus voltage data from voltage.db
        """
        if not DBManager.check_tables_in_db(
                PathManager.get_db_path(self._result_dir, self.db_name), DBNameConstant.TABLE_BUS_VOLTAGE):
            return []
        sql = "select syscnt, voltage from {}".format(DBNameConstant.TABLE_BUS_VOLTAGE)
        return DBManager.fetch_all_data(self.cur, sql)

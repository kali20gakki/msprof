#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_iteration import MsprofIteration
from msmodel.interface.ianalysis_model import IAnalysisModel
from msmodel.interface.view_model import ViewModel


class GeOpExecuteViewModel(ViewModel, IAnalysisModel):
    """
    Model of viewer for GE op execute
    """

    def __init__(self, params: dict) -> None:
        self._result_dir = params.get(StrConstant.PARAM_RESULT_DIR)
        self._iter_range = params.get(StrConstant.PARAM_ITER_ID)
        super().__init__(self._result_dir, DBNameConstant.DB_GE_HOST_INFO,
                         [DBNameConstant.TABLE_GE_HOST])

    def get_summary_data(self: any) -> list:
        """
        get summary data of Ge op execute
        :return:
        """
        sql = "select thread_id,op_name, op_type,event_type,start_time," \
              "(end_time-start_time) from {0} {where_condition}".format(DBNameConstant.TABLE_GE_HOST,
                                                                        where_condition=self._get_where_condition())
        if not (self.cur and self.conn):
            return []
        return DBManager.fetch_all_data(self.cur, sql)

    def get_timeline_data(self: any) -> list:
        """
        get timeline data of Ge op execute
        :return:
        """
        return self.get_summary_data()

    def _get_where_condition(self):
        return MsprofIteration(self._result_dir).get_condition_within_iteration(self._iter_range,
                                                                                time_start_key='start_time',
                                                                                time_end_key='end_time')

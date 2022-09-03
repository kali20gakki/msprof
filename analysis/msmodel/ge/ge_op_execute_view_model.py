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
        self._model_id = params.get(StrConstant.PARAM_MODEL_ID)
        self._index_id = params.get(StrConstant.PARAM_ITER_ID)
        super().__init__(self._result_dir, DBNameConstant.DB_GE_HOST_INFO,
                         [DBNameConstant.TABLE_GE_HOST])

    def get_summary_data(self: any) -> list:
        """
        get summary data of Ge op execute
        :return:
        """
        sql = "select thread_id,op_type,event_type,start_time/{0}," \
              "(end_time-start_time)/{0} from {1} {where_condition}".format(NumberConstant.MILLI_SECOND,
                                                                            DBNameConstant.TABLE_GE_HOST,
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
        return MsprofIteration(self._result_dir).get_condition_within_iteration(self._index_id,
                                                                                self._model_id,
                                                                                time_start_key='start_time',
                                                                                time_end_key='end_time')

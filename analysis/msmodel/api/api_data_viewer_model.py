#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_iteration import MsprofIteration
from msmodel.interface.ianalysis_model import IAnalysisModel
from msmodel.interface.view_model import ViewModel


class ApiDataViewModel(ViewModel):
    """
    class for api_viewer
    """

    def __init__(self, params: dict) -> None:
        self._result_dir = params.get(StrConstant.PARAM_RESULT_DIR)
        self._iter_range = params.get(StrConstant.PARAM_ITER_ID)
        super().__init__(self._result_dir, DBNameConstant.DB_API_EVENT,
                         [DBNameConstant.TABLE_API_DATA])

    def get_timeline_data(self: any) -> list:
        sql = "select struct_type, start, (end - start), " \
              "thread_id, level, id," \
              "item_id from {} {where_condition}".format(DBNameConstant.TABLE_API_DATA,
                                                                    where_condition=self._get_where_condition())
        return DBManager.fetch_all_data(self.cur, sql)

    def _get_where_condition(self):
        return MsprofIteration(self._result_dir).get_condition_within_iteration(self._iter_range,
                                                                                time_start_key='start',
                                                                                time_end_key='end')
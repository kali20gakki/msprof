#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

from common_func.common import CommonConstant
from common_func.common import CommonConstant
from common_func.db_manager import DBManager
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_iteration import MsprofIteration
from msmodel.interface.ianalysis_model import IAnalysisModel
from msmodel.interface.view_model import ViewModel


class AclModel(ViewModel, IAnalysisModel):
    """
    Model of viewer for acl data
    """

    def __init__(self, params: dict) -> None:
        self._result_dir = params.get(StrConstant.PARAM_RESULT_DIR)
        self._model_id = params.get(StrConstant.PARAM_MODEL_ID)
        self._index_id = params.get(StrConstant.PARAM_ITER_ID)
        super().__init__(self._result_dir, DBNameConstant.DB_ACL_MODULE,
                         [DBNameConstant.TABLE_ACL_DATA])

    def get_timeline_data(self: any) -> list:
        sql = "select api_name, start_time, (end_time-start_time) " \
              "as output_duration, process_id, thread_id, api_type " \
              "from {0} {where_condition} order by start_time".format(DBNameConstant.TABLE_ACL_DATA,
                                                                      where_condition=self._get_where_condition())
        return DBManager.fetch_all_data(self.cur, sql)

    def get_summary_data(self: any) -> list:
        sql = "select api_name, api_type, start_time, (end_time-start_time)/{0} " \
              "as output_duration, process_id, thread_id " \
              "from {1} {where_condition} order by start_time asc".format(NumberConstant.NS_TO_US,
                                                                          DBNameConstant.TABLE_ACL_DATA,
                                                                          where_condition=self._get_where_condition())
        return DBManager.fetch_all_data(self.cur, sql)

    def get_acl_total_time(self):
        search_data_sql = f"select sum(end_time-start_time) " \
                          f"from {DBNameConstant.TABLE_ACL_DATA} {self._get_where_condition()}"
        return self.cur.execute(search_data_sql).fetchone()[0]

    def get_acl_statistic_data(self):
        total_time = self.get_acl_total_time()
        if not total_time:
            return []

        search_data_sql = "select api_name, api_type, " \
                          "round({percent}*sum(end_time-start_time)/{total_time}, {accuracy}), " \
                          "sum((end_time-start_time)/{1}), count(*), " \
                          "sum((end_time-start_time)/{1})/count(*), " \
                          "min((end_time-start_time)/{1}), " \
                          "max((end_time-start_time)/{1}), " \
                          "process_id, thread_id from {0} " \
                          "{where_condition} " \
                          "group by api_name".format(DBNameConstant.TABLE_ACL_DATA,
                                                     NumberConstant.NS_TO_US,
                                                     percent=CommonConstant.PERCENT,
                                                     total_time=total_time,
                                                     accuracy=CommonConstant.ROUND_SIX,
                                                     where_condition=self._get_where_condition())
        return DBManager.fetch_all_data(self.cur, search_data_sql)

    def _get_where_condition(self):
        return MsprofIteration(self._result_dir).get_condition_within_iteration(self._index_id,
                                                                                self._model_id,
                                                                                time_start_key='start_time',
                                                                                time_end_key='end_time')

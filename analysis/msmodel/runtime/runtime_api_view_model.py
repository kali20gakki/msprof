#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_iteration import MsprofIteration
from msmodel.interface.ianalysis_model import IAnalysisModel
from msmodel.interface.view_model import ViewModel


class RuntimeApiViewModel(ViewModel, IAnalysisModel):
    def __init__(self, params: dict) -> None:
        self._result_dir = params.get(StrConstant.PARAM_RESULT_DIR)
        self._model_id = params.get(StrConstant.PARAM_MODEL_ID)
        self._index_id = params.get(StrConstant.PARAM_ITER_ID)
        super().__init__(self._result_dir, DBNameConstant.DB_RUNTIME,
                         [DBNameConstant.TABLE_API_CALL])

    def get_timeline_data(self: any) -> list:
        sql = "select api, entry_time, (exit_time - entry_time), " \
              "thread, data_size, memcpy_direction," \
              "stream_id, task_id from {} {where_condition}".format(DBNameConstant.TABLE_API_CALL,
                                                                    where_condition=self._get_where_condition())
        return DBManager.fetch_all_data(self.cur, sql)

    def get_summary_data(self: any) -> list:
        time_total = self.get_runtime_total_time()
        if not (time_total and time_total[0]):
            return []

        sql = "select api, (case when stream_id=65535 then 'N/A' else stream_id end), " \
              "round(1.0*sum(exit_time-entry_time)*{percent}/{0}, {accuracy}), " \
              "sum(exit_time-entry_time), count(*), " \
              "sum(exit_time-entry_time)/count(*), min(exit_time-entry_time), " \
              "max(exit_time-entry_time), {process_id}, thread " \
              "from {1} {where_condition} group by api ".format(time_total[0],
                                                                DBNameConstant.TABLE_API_CALL,
                                                                percent=NumberConstant.PERCENTAGE,
                                                                accuracy=NumberConstant.DECIMAL_ACCURACY,
                                                                process_id=InfoConfReader().get_json_pid_data(),
                                                                where_condition=self._get_where_condition())
        return DBManager.fetch_all_data(self.cur, sql)

    def get_runtime_total_time(self):
        search_data_sql = f"select sum(exit_time-entry_time) " \
                          f"from {DBNameConstant.TABLE_API_CALL} {self._get_where_condition()}"
        total_time = DBManager.fetch_all_data(self.cur, search_data_sql)
        return total_time[0] if total_time else []

    def _get_where_condition(self):
        return MsprofIteration(self._result_dir).get_condition_within_iteration(self._index_id,
                                                                                self._model_id,
                                                                                time_start_key='entry_time',
                                                                                time_end_key='exit_time')

"""
This script is used to create ai core operator summary db from other db.
Copyright Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
"""
import logging
import sqlite3

from common_func.utils import Utils
from msmodel.interface.view_model import ViewModel
from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msprof_iteration import MsprofIteration
from common_func.platform.chip_manager import ChipManager


class GetOpTableTsTime:
    """
    to get task time table for op counter
    """

    def __init__(self: any, sample_config: dict) -> None:
        """initialize function"""
        self.sample_config = sample_config
        self.iter_id = self.sample_config.get("iter_id")
        self.model_id = self.sample_config.get("model_id")
        self.project_path = sample_config.get("result_dir")
        self._task_time_table = self._get_task_time_table()

    @staticmethod
    def _get_aiv_task_sql() -> str:
        sql = "select {1}.task_id, {1}.stream_id, {1}.running, {1}.complete - {1}.running, '{0}', " \
              "{1}.index_id, batch_id from {1} order by running" \
            .format(Constant.TASK_TYPE_AIV, DBNameConstant.TABLE_HWTS_TASK_TIME)
        return sql

    @staticmethod
    def _get_db_name() -> str:
        """
        get db name
        :return:
        """
        if ChipManager().is_chip_v1():
            return DBNameConstant.DB_RUNTIME
        return DBNameConstant.DB_HWTS

    @staticmethod
    def _get_task_time_table() -> str:
        if ChipManager().is_chip_v1():
            return DBNameConstant.TABLE_RUNTIME_TASK_TIME
        return DBNameConstant.TABLE_HWTS_TASK_TIME

    def get_task_time_data(self: any) -> list:
        """
        get op counter task time data
        :return:
        """
        ai_core_data = self._get_ai_core_task_time()
        ai_cpu_data = self._get_ai_cpu_task_time()
        aiv_data = self._get_aiv_task_time()
        fetched_data = ai_cpu_data + ai_core_data + aiv_data
        if not fetched_data:
            logging.warning("Unable to find TaskTime data")
            return []
        # sort fetched_data by start time
        rts_data = sorted(fetched_data, key=lambda x: float(x[2]))
        return rts_data

    def get_op_ai_cpu_task_sql(self: any) -> str:
        """
        get ai cpu task time data for single op
        :return:
        """
        ai_cpu_sql = "select task_id, stream_id, sys_start*{MS_TO_NS}, (sys_end - sys_start)*{MS_TO_NS}, " \
                     "'{0}', {2}, batch_id from {1} " \
            .format(Constant.TASK_TYPE_AI_CPU, DBNameConstant.TABLE_AI_CPU_FROM_TS, self.iter_id,
                    MS_TO_NS=NumberConstant.MS_TO_NS)
        return ai_cpu_sql

    def _get_ai_core_task_time(self: any) -> list:
        model_view = ViewModel(self.project_path, self._get_db_name(), [self._task_time_table])
        if model_view.check_table():
            return model_view.get_sql_data(self._get_ai_core_sql())
        return []

    def _get_aiv_task_time(self: any) -> list:
        model_view = ViewModel(self.project_path, DBNameConstant.DB_HWTS_AIV, [DBNameConstant.TABLE_HWTS_TASK_TIME])
        if model_view.check_table():
            return model_view.get_sql_data(self._get_aiv_task_sql())
        return []

    def _get_ai_cpu_task_time(self: any) -> list:
        ai_cpu_time = []
        model_view = ViewModel(self.project_path, DBNameConstant.DB_AI_CPU, [DBNameConstant.TABLE_AI_CPU_FROM_TS])
        if model_view.check_table():
            ai_cpu_time.extend(model_view.get_sql_data(self._get_ai_cpu_task_sql()))
        return ai_cpu_time

    def _get_ai_core_sql(self: any) -> str:
        if ProfilingScene().is_operator():
            return self._get_op_ai_core_task_time_sql()
        return self._get_no_op_ai_core_task_time_sql()

    def _get_op_ai_core_task_time_sql(self: any) -> str:
        """
        get ai core task time sql without model id
        :return:
        """
        limit = 'where tasktype=0' if ChipManager().is_chip_v1() else ''
        sql = "select {1}.task_id, {1}.stream_id, {1}.running, {1}.complete - {1}.running, '{0}', " \
              "{1}.index_id, {1}.batch_id from {1} {limit} group by running order by running" \
            .format(Constant.TASK_TYPE_AI_CORE, self._task_time_table, limit=limit)
        return sql

    def _get_no_op_ai_core_task_time_sql(self: any) -> str:
        """
        get ai core task time sql with model id
        :return:
        """
        limit = 'and tasktype=0' if ChipManager().is_chip_v1() else ''
        sql = "select {1}.task_id, {1}.stream_id, {1}.running, {1}.complete - {1}.running, '{0}'," \
              " {1}.index_id, {1}.model_id, {1}.batch_id from {1} where " \
              "{1}.index_id={2} and {1}.model_id={3} {limit} order by running " \
            .format(Constant.TASK_TYPE_AI_CORE, self._task_time_table,
                    self.iter_id, self.model_id, limit=limit)
        if Utils.need_all_model_in_one_iter(self.project_path, self.model_id):
            # export all index data when pytorch graph no model_id set
            limit = 'where tasktype=0' if ChipManager().is_chip_v1() else ''
            sql = "select {1}.task_id, {1}.stream_id, {1}.running, {1}.complete - {1}.running, '{0}'," \
                  " {1}.index_id, {1}.model_id, {1}.batch_id from {1} " \
                  "{limit} order by running " \
                .format(Constant.TASK_TYPE_AI_CORE, self._task_time_table, limit=limit)
        return sql

    def _get_ai_cpu_task_sql(self: any) -> str:
        """
        get ai cpu task time sql
        :return: sqlite statement
        """
        if ProfilingScene().is_operator():
            return self.get_op_ai_cpu_task_sql()
        iter_time = MsprofIteration(self.project_path).get_iteration_time(self.iter_id, self.model_id)

        ai_cpu_sql = ''
        try:
            ai_cpu_sql = "select task_id, stream_id, sys_start*{MS_TO_NS}, (sys_end - sys_start)*{MS_TO_NS}, " \
                         "'{1}', {4}, {5}, batch_id from {0} where sys_start >= {2} and sys_end <= {3}" \
                .format(DBNameConstant.TABLE_AI_CPU, Constant.TASK_TYPE_AI_CPU,
                        iter_time[0][0] / NumberConstant.NS_TO_US,
                        iter_time[0][1] / NumberConstant.NS_TO_US,
                        self.iter_id, self.model_id,
                        MS_TO_NS=NumberConstant.MS_TO_NS)
        except ZeroDivisionError as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return ai_cpu_sql
        return ai_cpu_sql

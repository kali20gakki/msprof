#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.

import logging

from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_iteration import MsprofIteration
from common_func.platform.chip_manager import ChipManager
from common_func.utils import Utils
from msmodel.interface.view_model import ViewModel
from msmodel.step_trace.ts_track_model import TsTrackModel


class GetOpTableTsTime:
    """
    to get task time table for op counter
    """

    def __init__(self: any, sample_config: dict) -> None:
        """initialize function"""
        self.sample_config = sample_config
        self.iter_range = self.sample_config.get(StrConstant.PARAM_ITER_ID)
        self.project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)

    @staticmethod
    def _get_aiv_task_sql() -> str:
        return "select {1}.task_id, {1}.stream_id, {1}.running, {1}.complete - {1}.running, '{0}', " \
               "{1}.index_id, batch_id,{subtask_id} from {1} order by running" \
            .format(Constant.TASK_TYPE_AI_CORE, DBNameConstant.TABLE_HWTS_TASK_TIME,
                    subtask_id=NumberConstant.DEFAULT_FFTS_SUBTASK_ID)

    @staticmethod
    def _get_acsq_task_sql() -> str:
        sql = "select task_id, stream_id, start_time, task_time,task_type,index_id,model_id,batch_id,{subtask_id} " \
              "from {} order by start_time".format(DBNameConstant.TABLE_ACSQ_TASK_TIME,
                                                   subtask_id=NumberConstant.DEFAULT_FFTS_SUBTASK_ID)
        if ProfilingScene().is_operator():
            sql = "select task_id, stream_id, start_time, task_time,task_type,index_id, batch_id,{subtask_id} " \
                  "from {} order by start_time".format(DBNameConstant.TABLE_ACSQ_TASK_TIME,
                                                       subtask_id=NumberConstant.DEFAULT_FFTS_SUBTASK_ID)
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

    @staticmethod
    def _is_tradition_task(data):
        if data.context_id == Constant.GE_OP_MODEL_ID:
            return True
        return False

    @staticmethod
    def _is_mix_task(data):
        if data.context_id == 0 and (
                data.task_type == Constant.TASK_TYPE_MIX_AIC or data.task_type == Constant.TASK_TYPE_MIX_AIV):
            return True
        return False

    def get_task_time_data(self: any, ge_data=None) -> list:
        """
        get op counter task time data
        :return:
        """
        if ge_data is None:
            ge_data = []
        logging.info("Start to get ffts profile task data.")
        task_data = self._get_task_time_data_by_ffts_profile(ge_data)
        if not task_data:
            logging.info("Start to get task data.")
            task_data = self._get_task_time_data_by_hwts()
            if not task_data:
                logging.warning("Unable to find TaskTime data")
                return []
        # sort fetched_data by start time
        return sorted(task_data, key=lambda x: float(x[2]))

    def _get_task_time_data_by_ffts_profile(self, ge_data):
        result_list = []
        tradition_task, mix_task, subgraph_task = self._get_ge_data(ge_data)
        result_list.extend(self._get_sub_task_time(subgraph_task))
        result_list.extend(self._get_sub_task_time(mix_task))
        result_list.extend(self._get_acsq_task_time(tradition_task))
        return result_list

    def _get_ge_data(self: any, ge_data) -> tuple:
        tradition_task = []
        subgraph_task = []
        mix_task = []
        for data in ge_data:
            if self._is_tradition_task(data):
                tradition_task.append(data)
            elif self._is_mix_task(data):
                mix_task.append(data)
            else:
                subgraph_task.append(data)
        return tradition_task, mix_task, subgraph_task

    def _get_task_time_data_by_hwts(self):
        task_data = []
        task_data.extend(self._get_aicore_and_aicpu())
        task_data.extend(self._get_aicpu_for_chip2())
        task_data.extend(self._get_aiv_task_time())
        return task_data

    def _get_task_time_data(self, db_name, table_list, sql):
        model_view = ViewModel(self.project_path, db_name, table_list)
        if model_view.check_table():
            return model_view.get_sql_data(sql)
        return []

    def _get_aicore_and_aicpu(self: any) -> list:
        return self._get_task_time_data(self._get_db_name(), [self._get_task_time_table()],
                                        self._get_ai_core_sql())

    def _get_sub_task_time(self: any, ge_data) -> list:
        if not ge_data:
            return []
        ge_data_set = set()
        for data in ge_data:
            task_key = "{}-{}-{}".format(data.stream_id, data.task_id, data.context_id)
            ge_data_set.add(task_key)
        subtask_data = self._get_task_time_data(DBNameConstant.DB_SOC_LOG, [DBNameConstant.TABLE_SUBTASK_TIME],
                                                self._get_sub_task_sql())
        return [data for data in subtask_data if "{}-{}-{}".format(data[1], data[0], data[8]) in ge_data_set]

    def _get_acsq_task_time(self: any, ge_data) -> list:
        if not ge_data:
            return []
        ge_data_dict = dict()
        for data in ge_data:
            task_key = "{}-{}".format(data.stream_id, data.task_id)
            ge_data_dict[task_key] = data.task_type
        task_data = self._get_task_time_data(DBNameConstant.DB_ACSQ, [DBNameConstant.TABLE_ACSQ_TASK_TIME],
                                             self._get_acsq_task_sql())
        res_data = []
        for data in task_data:
            task_type = ge_data_dict.get("{}-{}".format(data[1], data[0]))
            if task_type is not None:
                res_data.append(data[:4] + (task_type,) + data[5:])
        return res_data

    def _get_ai_core_task_time(self: any) -> list:
        return self._get_task_time_data(self._get_db_name(), [self._get_task_time_table()], self._get_ai_core_sql())

    def _get_aiv_task_time(self: any) -> list:
        return self._get_task_time_data(DBNameConstant.DB_HWTS_AIV, [DBNameConstant.TABLE_HWTS_TASK_TIME],
                                        self._get_aiv_task_sql())

    def _get_aicpu_for_chip2(self: any) -> list:
        ai_cpu_time = []
        model_view = ViewModel(self.project_path, DBNameConstant.DB_AI_CPU, [DBNameConstant.TABLE_AI_CPU_FROM_TS])
        if model_view.check_table() and ChipManager().is_chip_v2():
            ai_cpu_time.extend(model_view.get_sql_data(self._get_ai_cpu_task_sql()))
        return self._update_aicpu_task_index_id(ai_cpu_time)

    def _update_aicpu_task_index_id(self, ai_cpu_time):
        if ProfilingScene().is_operator():
            return ai_cpu_time
        with TsTrackModel(self.project_path, DBNameConstant.DB_STEP_TRACE,
                          [DBNameConstant.TABLE_STEP_TRACE_DATA]) as _trace:
            step_trace_data = _trace.get_step_end_list_with_iter_range(self.iter_range)
        ai_cpu_time = [
            ai_cpu[:5] +
            (MsprofIteration(self.project_path).get_iter_id_within_iter_range(step_trace_data, ai_cpu[2],
                                                                              self.iter_range),) + ai_cpu[6:]
            for ai_cpu in ai_cpu_time
        ]
        return ai_cpu_time

    def _get_ai_core_sql(self: any) -> str:
        if ProfilingScene().is_operator():
            return self._get_op_ai_core_task_time_sql()
        return self._get_network_ai_core_task_time_sql()

    def _get_op_ai_core_task_time_sql(self: any) -> str:
        """
        get ai core task time sql without model id
        :return:
        """
        return "select {1}.task_id, {1}.stream_id, {1}.running, {1}.complete - {1}.running, '{0}', " \
               "{1}.index_id, {1}.batch_id, {subtask_id} from {1} group by running order by running" \
            .format(Constant.TASK_TYPE_AI_CORE, self._get_task_time_table(),
                    subtask_id=NumberConstant.DEFAULT_FFTS_SUBTASK_ID)

    def _get_network_ai_core_task_time_sql(self: any) -> str:
        """
        get ai core task time sql with model id
        :return:
        """
        sql = "select {1}.task_id, {1}.stream_id, {1}.running, {1}.complete - {1}.running, '{0}'," \
              " {1}.index_id, {1}.model_id, {1}.batch_id, {subtask_id} from {1} where " \
              "{1}.index_id>={2} and {1}.index_id<={3} and {1}.model_id={4} order by running " \
            .format(Constant.TASK_TYPE_AI_CORE, self._get_task_time_table(),
                    self.iter_range.iteration_start, self.iter_range.iteration_end, self.iter_range.model_id,
                    subtask_id=NumberConstant.DEFAULT_FFTS_SUBTASK_ID)
        if Utils.need_all_model_in_one_iter(self.project_path, self.iter_range.model_id):
            # export all index data when pytorch graph no model_id set
            sql = "select {1}.task_id, {1}.stream_id, {1}.running, {1}.complete - {1}.running, '{0}'," \
                  " {1}.index_id, {1}.model_id, {1}.batch_id, {subtask_id} from {1} " \
                  "order by running " \
                .format(Constant.TASK_TYPE_AI_CORE, self._get_task_time_table(),
                        subtask_id=NumberConstant.DEFAULT_FFTS_SUBTASK_ID)
        return sql

    def _get_op_ai_cpu_task_sql(self: any) -> str:
        """
        get ai cpu task time data for single op
        :return:
        """
        return "select task_id, stream_id, sys_start*{MS_TO_NS}, (sys_end - sys_start)*{MS_TO_NS}, " \
               "'{0}', {2}, batch_id,{subtask_id} from {1} " \
            .format(Constant.TASK_TYPE_AI_CPU, DBNameConstant.TABLE_AI_CPU_FROM_TS, self.iter_range.iteration_id,
                    MS_TO_NS=NumberConstant.MS_TO_NS,
                    subtask_id=NumberConstant.DEFAULT_FFTS_SUBTASK_ID)

    def _get_ai_cpu_task_sql(self: any) -> str:
        """
        get ai cpu task time sql
        :return: sqlite statement
        """
        if ProfilingScene().is_operator():
            return self._get_op_ai_cpu_task_sql()
        iter_time = MsprofIteration(self.project_path).get_iter_interval(self.iter_range, NumberConstant.MILLI_SECOND)

        ai_cpu_sql = ''
        try:
            ai_cpu_sql = "select task_id, stream_id, sys_start*{MS_TO_NS}, (sys_end - sys_start)*{MS_TO_NS}, " \
                         "'{1}', {4}, {5}, batch_id, {subtask_id} from {0} where sys_start >= {2} and sys_end <= {3}" \
                .format(DBNameConstant.TABLE_AI_CPU_FROM_TS, Constant.TASK_TYPE_AI_CPU,
                        iter_time[0], iter_time[1],
                        self.iter_range.iteration_id, self.iter_range.model_id,
                        MS_TO_NS=NumberConstant.MS_TO_NS,
                        subtask_id=NumberConstant.DEFAULT_FFTS_SUBTASK_ID)
        except ZeroDivisionError as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
        return ai_cpu_sql

    def _get_sub_task_sql(self):
        # the subtask type of ai core or ai vector that can not be distinguish by GE.
        return "select task_id, stream_id, start_time, dur_time, " \
               "(case when subtask_type='AIC' then 'AI_CORE' when subtask_type='AIV' then 'AI_VECTOR_CORE' " \
               "else subtask_type end), {1}, {2}, 0, subtask_id from {0} order by start_time" \
            .format(DBNameConstant.TABLE_SUBTASK_TIME, self.iter_range.iteration_id, self.iter_range.model_id)

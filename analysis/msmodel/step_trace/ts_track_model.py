#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.

from abc import ABC
from functools import partial

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.empty_class import EmptyClass
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.path_manager import PathManager
from msmodel.interface.base_model import BaseModel
from msmodel.interface.view_model import ViewModel
from profiling_bean.db_dto.step_trace_dto import IterationRange
from profiling_bean.db_dto.step_trace_dto import StepTraceDto
from profiling_bean.db_dto.step_trace_ge_dto import StepTraceGeDto
from profiling_bean.db_dto.time_section_dto import TimeSectionDto


class TsTrackModel(BaseModel, ABC):
    """
    acsq task model class
    """
    TS_AI_CPU_TYPE = 1

    @staticmethod
    def __aicpu_in_time_range(data, min_timestamp, max_timestamp):
        return min_timestamp <= data[2] <= max_timestamp

    def flush(self: any, table_name: str, data_list: list) -> None:
        """
        flush acsq task data to db
        :param data_list:acsq task data list
        :return: None
        """
        self.insert_data_to_db(table_name, data_list)

    def create_table(self: any, table_name: str) -> None:
        """
        create table
        """
        if DBManager.judge_table_exist(self.cur, table_name):
            DBManager.drop_table(self.conn, table_name)
        table_map = "{0}Map".format(table_name)
        sql = DBManager.sql_create_general_table(table_map, table_name, self.TABLES_PATH)
        DBManager.execute_sql(self.conn, sql)

    def get_ai_cpu_data(self: any, iter_time_range) -> list:
        """
        get ai cpu data
        :param iter_time_range: iteration time range
        :return: ai cpu with state
        """
        if not DBManager.check_tables_in_db(PathManager.get_db_path(self.result_dir, DBNameConstant.DB_STEP_TRACE),
                                            DBNameConstant.TABLE_TASK_TYPE):
            return []

        sql = "select stream_id, task_id, timestamp, task_state from {0} " \
              "where task_type={1} order by timestamp ".format(DBNameConstant.TABLE_TASK_TYPE, self.TS_AI_CPU_TYPE)
        ai_cpu_with_state = DBManager.fetch_all_data(self.cur, sql)

        for index, datum in enumerate(ai_cpu_with_state):
            ai_cpu_with_state[index] = list(datum)
            # index 2 is timestamp
            ai_cpu_with_state[index][2] = int(datum[2])

        if iter_time_range:
            min_timestamp = min(iter_time_range)
            max_timestamp = max(iter_time_range)

            # data index 2 is timestamp
            ai_cpu_with_state = list(filter(partial(self.__aicpu_in_time_range, min_timestamp=min_timestamp,
                                                    max_timestamp=max_timestamp), ai_cpu_with_state))

        return ai_cpu_with_state

    def get_step_trace_data(self: any) -> list:
        """
        get step trace data
        """
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_STEP_TRACE_DATA) or \
           not DBManager.judge_row_exist(self.cur, DBNameConstant.TABLE_STEP_TRACE_DATA):
            return []
        sql = "select model_id, index_id, iter_id, step_start, step_end from {0}".format(
            DBNameConstant.TABLE_STEP_TRACE_DATA)
        step_trace_data = DBManager.fetch_all_data(self.cur, sql, dto_class=StepTraceDto)
        return step_trace_data

    def get_max_index_id_with_model(self, model_id):
        """
        get the max iteration id of the model id.
        """
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_STEP_TRACE_DATA) or \
           not DBManager.judge_row_exist(self.cur, DBNameConstant.TABLE_STEP_TRACE_DATA):
            return EmptyClass()
        sql = f'select max(index_id) as index_id from {DBNameConstant.TABLE_STEP_TRACE_DATA} where model_id=?'
        return DBManager.fetchone(self.cur, sql, (model_id,), dto_class=StepTraceDto)

    def get_step_syscnt_range_by_iter_range(self, iteration: IterationRange):
        """
        get step time range by the iteration range.
        """
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_STEP_TRACE_DATA) or \
           not DBManager.judge_row_exist(self.cur, DBNameConstant.TABLE_STEP_TRACE_DATA):
            return EmptyClass()
        sql = f"select min(step_start) as step_start, max(step_end) as step_end " \
              f"from {DBNameConstant.TABLE_STEP_TRACE_DATA} where model_id=? and index_id>=? and index_id<=?"
        return DBManager.fetchone(self.cur, sql, (iteration.model_id, *iteration.get_iteration_range()),
                                  dto_class=StepTraceDto)

    def get_step_end_list_with_iter_range(self, iteration: IterationRange):
        """
        get step trace within the range of iteration.
        """
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_STEP_TRACE_DATA) or \
           not DBManager.judge_row_exist(self.cur, DBNameConstant.TABLE_STEP_TRACE_DATA):
            return []
        sql = f"select index_id, step_end from {DBNameConstant.TABLE_STEP_TRACE_DATA} " \
              f"where model_id=? and index_id>=? and index_id<=? order by step_end"
        return DBManager.fetch_all_data(self.cur, sql, (iteration.model_id, *iteration.get_iteration_range()),
                                        dto_class=StepTraceDto)


class TsTrackViewModel(ViewModel):
    def __init__(self: any, path: str) -> None:
        super().__init__(path, DBNameConstant.DB_STEP_TRACE, [])

    def get_hccl_operator_exe_data(self) -> list:
        if not self.attach_to_db(DBNameConstant.DB_GE_INFO):
            return []
        sql = "SELECT t1.model_id model_id, t1.index_id index_id, t1.stream_id stream_id, t1.task_id task_id, " \
              "t1.tag_id tag_id, t1.timestamp timestamp, t2.op_name op_name, t2.op_type op_type " \
              "FROM ( SELECT model_id, index_id, tag_id, stream_id, task_id-1 AS task_id, timestamp " \
              "FROM {} WHERE tag_id>=10000 ) t1 LEFT JOIN ( " \
              "SELECT model_id, index_id, stream_id, task_id, op_name, op_type FROM {} WHERE task_type='{}' ) t2 " \
              "ON t1.model_id=t2.model_id AND (t1.index_id=t2.index_id OR t2.index_id=0 ) " \
              "AND t1.stream_id = t2.stream_id AND t1.task_id = t2.task_id ORDER BY t1.timestamp".format(
            DBNameConstant.TABLE_STEP_TRACE, DBNameConstant.TABLE_GE_TASK, Constant.TASK_TYPE_HCCL)
        return DBManager.fetch_all_data(self.cur, sql, dto_class=StepTraceGeDto)

    def get_ai_cpu_data(self) -> list:
        sql = "SELECT stream_id, task_id, timestamp, task_state FROM {} where " \
              "task_type=1 and (task_state=1 or task_state=2) order by timestamp".format(
            DBNameConstant.TABLE_TASK_TYPE)
        return DBManager.fetch_all_data(self.cur, sql)

    def get_iter_time_data(self) -> list:
        sql = "select model_id, index_id ,step_start as start_time, step_end as end_time " \
              "from {} order by end_time".format(DBNameConstant.TABLE_STEP_TRACE_DATA)
        return DBManager.fetch_all_data(self.cur, sql, dto_class=TimeSectionDto)

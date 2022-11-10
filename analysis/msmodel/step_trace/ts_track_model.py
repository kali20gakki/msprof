#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.

from abc import ABC
from functools import partial
from itertools import chain

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.msprof_iteration import MsprofIteration
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.path_manager import PathManager
from msmodel.interface.base_model import BaseModel
from msmodel.interface.view_model import ViewModel
from profiling_bean.db_dto.time_section_dto import TimeSectionDto
from profiling_bean.db_dto.step_trace_dto import StepTraceDto


class TsTrackModel(BaseModel, ABC):
    """
    acsq task model class
    """
    TS_AI_CPU_TYPE = 1

    @staticmethod
    def __aicpu_in_time_range(data, min_timestamp, max_timestamp):
        return min_timestamp <= InfoConfReader().time_from_syscnt(data[2],
                                                                  NumberConstant.MICRO_SECOND) <= max_timestamp

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

    def get_ai_cpu_data(self: any, model_id: int, index_id: int) -> list:
        """
        get ai cpu data
        :param model_id: model id
        :param index_id: index id
        :return: ai cpu with state
        """
        if not DBManager.check_tables_in_db(PathManager.get_db_path(self.result_dir, DBNameConstant.DB_STEP_TRACE),
                                            DBNameConstant.TABLE_TASK_TYPE):
            return []

        iter_time_range = list(chain.from_iterable(
            MsprofIteration(self.result_dir).get_iteration_time(index_id, model_id)))
        sql = "select stream_id, task_id, timestamp, " \
              "task_state from {0} where task_type={1} order by timestamp ".format(
            DBNameConstant.TABLE_TASK_TYPE,
            self.TS_AI_CPU_TYPE)
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

    def check_parallel(self: any) -> list:
        """
        check parallel
        """
        if not self.check_table():
            return []

        # one item is enough for checking parallel, and limit 0, 1 can improve speed
        sql = "select * from {0} t1 inner join " \
              "(select * from {0}) t2 " \
              "where t1.step_end > t2.step_start and t1.step_end < t2.step_end limit 0, 1".format(
            DBNameConstant.TABLE_STEP_TRACE_DATA)
        check_res = DBManager.fetch_all_data(self.cur, sql)
        return check_res

    def get_step_trace_data(self: any) -> list:
        """
        get step trace data
        """
        sql = "select model_id, index_id, iter_id, step_start, step_end from {0}".format(
            DBNameConstant.TABLE_STEP_TRACE_DATA)
        step_trace_data = DBManager.fetch_all_data(self.cur, sql, dto_class=StepTraceDto)
        return step_trace_data


class TsTrackViewModel(ViewModel):
    def __init__(self: any, path: str) -> None:
        super().__init__(path, DBNameConstant.DB_STEP_TRACE, [])

    def get_hccl_operator_exe_data(self) -> list:
        if not self.attach_to_db(DBNameConstant.DB_GE_INFO):
            return []
        if not self.attach_to_db(DBNameConstant.DB_GE_HASH):
            return []
        sql = "select t1.model_id, t1.index_id, case when t3.hash_value is not null then t3.hash_value " \
              "else t2.op_name end, t2.op_type, t1.start_time, t1.end_time from (" \
              "select t.model_id, t.index_id, t.flag, sum(case when tag=0 then timestamp else 0 end) start_time, " \
              "sum(case when tag=1 then timestamp else 0 end) end_time, " \
              "sum(case when tag=1 then stream_id else 0 end) stream_id, " \
              "sum(case when tag = 1 then task_id - 1 else 0 end) task_id " \
              "from( select model_id, index_id, tag_id%2 AS tag, stream_id, task_id, timestamp, " \
              "row_number() over(partition by model_id, index_id, tag_id%2 order by timestamp) as flag from {} " \
              "where tag_id >= 10000) t group by t.model_id, t.index_id, t.flag)t1 LEFT JOIN (" \
              "select model_id, index_id, stream_id, task_id, op_name, op_type from {} where task_type=4)t2 " \
              "on t1.model_id=t2.model_id and (t1.index_id=t2.index_id or t2.index_id=0) and " \
              "t1.stream_id=t2.stream_id and t1.task_id=t2.task_id " \
              "left join (select hash_key, hash_value from {})t3 on t2.op_name=t3.hash_key order by start_time".format(
                DBNameConstant.TABLE_STEP_TRACE,
                DBNameConstant.TABLE_GE_TASK, DBNameConstant.TABLE_GE_HASH)
        return DBManager.fetch_all_data(self.cur, sql)

    def get_ai_cpu_op_data(self) -> list:
        sql = "select t1.stream_id, t1.task_id, t1.grp, " \
              "sum(case when t1.task_state=1 then t1.timestamp else 0 end) as start_time, " \
              "sum(case when t1.task_state=2 then t1.timestamp else 0 end) as end_time " \
              "from (select t.timestamp, t.stream_id, t.task_id, t.task_state, " \
              "row_number() over(partition by t.stream_id, t.task_id,t.task_state order by t.timestamp) as grp " \
              "from (select timestamp,stream_id,task_id,task_state,  " \
              "lag(task_state, 1, 2) over(partition by stream_id,task_id order by timestamp) state " \
              "from {} where task_type=1 and task_state <>0)t where t.task_state+t.state=3)t1 " \
              "group by t1.stream_id, t1.task_id, t1.grp order by start_time desc".format(
                DBNameConstant.TABLE_TASK_TYPE)
        return DBManager.fetch_all_data(self.cur, sql, dto_class=TimeSectionDto)

    def get_iter_time_data(self) -> list:
        sql = "select model_id, index_id ,step_start as start_time, step_end as end_time " \
              "from {} order by end_time".format(DBNameConstant.TABLE_STEP_TRACE_DATA)
        return DBManager.fetch_all_data(self.cur, sql, dto_class=TimeSectionDto)

#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.str_constant import StrConstant
from msmodel.interface.parser_model import ParserModel
from msmodel.interface.view_model import ViewModel


class ParallelModel(ParserModel):
    def __init__(self: any, result_dir: str) -> None:
        super().__init__(result_dir, DBNameConstant.DB_PARALLEL,
                         [DBNameConstant.TABLE_PARALLEL_STRATEGY, DBNameConstant.TABLE_HCCL_OPERATOR_OVERLAP,
                          DBNameConstant.TABLE_COMPUTATION_TIME])

    def flush(self: any, table_name: str, data_list: list) -> None:
        """
        flush data to db
        """
        self.insert_data_to_db(table_name, data_list)


class ParallelViewModel(ViewModel):
    def __init__(self: any, path: str) -> None:
        super().__init__(path, DBNameConstant.DB_PARALLEL, [])

    def get_parallel_table_name(self: any) -> str:
        sql = "select parallel_mode from {}".format(DBNameConstant.TABLE_PARALLEL_STRATEGY)
        if not DBManager.fetch_all_data(self.cur, sql):
            return Constant.NA
        parallel_mode = DBManager.fetch_all_data(self.cur, sql)[0]
        if not parallel_mode:
            return Constant.NA
        return StrConstant.PARALLEL_TABLE_NAME_MAPPING.get(parallel_mode[0], Constant.NA)

    def get_parallel_index_data(self: any, tabel_name: str, rank_id: any, device_id: int) -> list:
        if rank_id == Constant.NA:
            rank_id = "null"
        if tabel_name == DBNameConstant.TABLE_CLUSTER_DATA_PARALLEL:
            sql = "select {0} rank_id, {1} device_id, t1.model_id, t1.index_id, t2.step_time, t2.computation_time, " \
                  "t1.pure_communication_time, t1.communication_time, t1.interval_of_communication_time, " \
                  "t1.hccl_op_num from (select t.model_id, t.index_id, sum(t.communication_time) communication_time, " \
                  "sum(t.pure_communication_time) pure_communication_time, count(0) hccl_op_num, " \
                  "sum(t.start_time-t.last_time)interval_of_communication_time from (select model_id, index_id, " \
                  "lag(end_time,1,start_time) over(partition by model_id, index_id order by start_time) last_time, " \
                  "start_time, end_time, end_time-start_time communication_time, " \
                  "end_time-start_time-overlap_time pure_communication_time from {2})t group by t.model_id, " \
                  "t.index_id)t1 left join {3} t2 on t1.model_id=t2.model_id and t1.index_id=t2.index_id".format(
                rank_id, device_id,
                DBNameConstant.TABLE_HCCL_OPERATOR_OVERLAP,
                DBNameConstant.TABLE_COMPUTATION_TIME)
        elif tabel_name == DBNameConstant.TABLE_CLUSTER_MODEL_PARALLEL:
            sql = "select {0} rank_id, {1} device_id, t1.model_id, t1.index_id, t2.step_time, t2.computation_time, " \
                  "t1.pure_communication_time, t1.communication_time from(select model_id, index_id, " \
                  "sum(end_time-start_time) communication_time, " \
                  "sum(end_time-start_time-overlap_time) pure_communication_time FROM {2} group by " \
                  "model_id, index_id ) t1 left join {3} t2 ON t1.model_id = t2.model_id and " \
                  "t1.index_id = t2.index_id".format(
                rank_id, device_id,
                DBNameConstant.TABLE_HCCL_OPERATOR_OVERLAP,
                DBNameConstant.TABLE_COMPUTATION_TIME)
        else:
            sql = "select {0} rank_id, {1} device_id, t1.model_id, t1.index_id, t2.step_time, t2.computation_time, " \
                  "t1.pure_communication_time, t1.communication_time, t1.pure_communication_time_only_revice, " \
                  "t1.pure_communication_time_except_revice from(select model_id, index_id, " \
                  "sum(end_time-start_time) communication_time, " \
                  "sum(end_time-start_time-overlap_time) pure_communication_time, " \
                  "sum(case when op_type='Receive' then end_time-start_time-overlap_time else 0 end) " \
                  "pure_communication_time_only_revice, sum(case when op_type<>'Receive' then " \
                  "end_time-start_time-overlap_time else 0 end) pure_communication_time_except_revice from {2} " \
                  "group by model_id, index_id)t1 left join {3} t2 ON t1.model_id = t2.model_id " \
                  "and t1.index_id = t2.index_id".format(
                rank_id, device_id,
                DBNameConstant.TABLE_HCCL_OPERATOR_OVERLAP,
                DBNameConstant.TABLE_COMPUTATION_TIME)
        return DBManager.fetch_all_data(self.cur, sql)

    def get_parallel_strategy_data(self: any) -> list:
        sql = "select * from {}".format(DBNameConstant.TABLE_PARALLEL_STRATEGY)
        return DBManager.fetch_all_data(self.cur, sql)

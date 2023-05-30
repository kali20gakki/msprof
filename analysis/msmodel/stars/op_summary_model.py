#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.

import logging
import sqlite3

from common_func.common import CommonConstant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.path_manager import PathManager
from common_func.ms_constant.number_constant import NumberConstant
from msmodel.interface.ianalysis_model import IAnalysisModel
from msmodel.interface.view_model import ViewModel
from msmodel.stars.acsq_task_model import AcsqTaskModel
from msmodel.stars.ffts_log_model import FftsLogModel
from profiling_bean.db_dto.ge_op_info_dto import GeOpInfoDto


class OpSummaryModel(ViewModel, IAnalysisModel):
    """
    class used to operate summary db
    """

    def __init__(self: any, sample_config: dict) -> None:
        super().__init__(sample_config.get("result_dir"), DBNameConstant.DB_AICORE_OP_SUMMARY, [])
        self.conn = None
        self.cur = None
        self.result_dir = sample_config.get("result_dir")
        self.iter_id = sample_config.get("iter_id")
        self.model_id = sample_config.get("model_id")

    def create_table(self: any) -> None:
        """
        create op summary db and table
        :return:
        """
        self.create_ge_summary_table()
        self.create_ge_tensor_table()
        self.create_ai_core_metrics_table()
        self.create_task_time_table()
        self.sql_commit()

    def create_ge_summary_table(self: any) -> None:
        """
        create ge summary table
        """
        if not self.attach_to_db(DBNameConstant.DB_GE_INFO):
            logging.warning("unable to create ge summary table, because attach db of ge failed.")
            return
        ge_merge_sql, sql_param = self._get_ge_sql()
        create_ge_summary_sql = "create table if not exists ge_summary as {}".format(ge_merge_sql)
        DBManager.execute_sql(self.conn, create_ge_summary_sql, sql_param)

    def create_ge_tensor_table(self: any) -> None:
        """
        create ge tensor table
        :return: None
        """
        ge_tensor_sql = "select * from {0} where (index_id={1} or index_id=0)" \
            .format(DBNameConstant.TABLE_GE_TENSOR, self.iter_id)
        create_ge_tensor_sql = "create table if not exists {0} as {1}" \
            .format(DBNameConstant.TABLE_SUMMARY_TENSOR, ge_tensor_sql)
        DBManager.execute_sql(self.conn, create_ge_tensor_sql)

    def create_ai_core_metrics_table(self: any) -> None:
        """
        create ai core metrics table
        :return: None
        """
        if not self.attach_to_db(DBNameConstant.DB_RUNTIME):
            logging.warning("unable to create ai core metrics table, because attach db of runtime failed.")
            return
        if DBManager.check_tables_in_db(self.get_db_path(DBNameConstant.DB_RUNTIME),
                                        CommonConstant.METRICS_SUMMARY_TABLE):
            sql = "create table if not exists ai_core_metrics " \
                  "as select * from {0}".format(CommonConstant.METRICS_SUMMARY_TABLE)
        else:
            logging.warning("unable to create ai core metrics table, because table is not found.")
            return
        DBManager.execute_sql(self.conn, sql)

    def create_task_time_table(self: any) -> None:
        """
        create task time table
        :return: None
        """
        create_table_sql = DBManager.sql_create_general_table("StarsTaskTimeMap",
                                                              DBNameConstant.TABLE_RUNTIME_TASK_TIME, self.TABLES_PATH)
        if not create_table_sql:
            logging.error("unable to create task time table, generate sql statement failed!")
            return
        DBManager.execute_sql(self.conn, create_table_sql)
        data = self.get_task_time_data()
        if not data:
            logging.warning("unable to create task time table, because no task data found.")
            return
        self.insert_data_to_db(DBNameConstant.TABLE_RUNTIME_TASK_TIME, data)

    def get_task_time_data(self: any) -> list:
        """
        get task time data
        :return: task time data
        """
        data = self._get_ffts_task_data()
        data.append(self._get_acsq_task_data())
        return data

    def get_db_path(self: any, db_name: str) -> str:
        """
        get database path
        :param db_name: db name
        :return: path of db
        """
        return PathManager.get_db_path(self.result_dir, db_name)

    def sql_commit(self: any) -> None:
        """
        sql commit
        """
        self.conn.commit()

    def get_summary_data(self: any) -> str:
        return self.__module__

    def get_timeline_data(self: any) -> str:
        return self.__module__

    def _get_ge_sql(self: any) -> tuple:
        ge_sql = "SELECT model_id, task_id, stream_id, " \
                 "op_name, op_type, block_dim, task_type, timestamp " \
                 "from {0} " \
                 "where (index_id=? or index_id=0) " \
                 "and model_id=?" \
            .format(DBNameConstant.TABLE_GE_TASK)
        return ge_sql, (self.iter_id, self.model_id)

    def _get_acsq_task_data(self: any) -> list:
        try:
            with AcsqTaskModel(self.result_dir, DBNameConstant.DB_SOC_LOG, []) as acsq_task_model:
                return acsq_task_model.get_ffts_type_data()
        except sqlite3.Error as err:
            logging.error("Get acsq task data failed! %s", err)
            return []

    def _get_ffts_task_data(self: any) -> list:
        try:
            with FftsLogModel(self.result_dir, DBNameConstant.DB_SOC_LOG, []) as ffts_log_model:
                return ffts_log_model.get_ffts_task_data()
        except sqlite3.Error as err:
            logging.error("Get ffts task data failed! %s", err)
            return []
        finally:
            pass

    def get_compute_op_data(self: any) -> list:
        """Get compute ops info for critical path analysis"""
        sql = "select {1}.model_id, {0}.task_id, {0}.stream_id, op_name, {1}.op_type, {1}.task_type, " \
              "start_time, duration_time " \
              "from {0} inner join {1} on {0}.task_id={1}.task_id and {0}.stream_id = {1}.stream_id " \
              "and {0}.task_type = {1}.task_type and {0}.batch_id={1}.batch_id " \
              "and ({1}.context_id={0}.subtask_id or ({1}.context_id={context_id} and subtask_id={subtask_id})) " \
              "order by start_time" \
            .format(DBNameConstant.TABLE_SUMMARY_TASK_TIME, DBNameConstant.TABLE_SUMMARY_GE,
                    NS_TO_US=NumberConstant.NS_TO_US,
                    context_id=NumberConstant.DEFAULT_GE_CONTEXT_ID,
                    subtask_id=NumberConstant.DEFAULT_FFTS_SUBTASK_ID, )
        return DBManager.fetch_all_data(self.cur, sql, dto_class=GeOpInfoDto)

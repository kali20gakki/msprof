#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.

import logging
import os
import sqlite3

from analyzer.get_op_table_task_time import GetOpTableTsTime
from analyzer.op_common_function import OpCommonFunc
from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.ai_stack_data_check_manager import AiStackDataCheckManager
from common_func.common import CommonConstant
from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_exception import ProfException
from common_func.msprof_iteration import MsprofIteration
from common_func.path_manager import PathManager
from common_func.platform.chip_manager import ChipManager
from msconfig.config_manager import ConfigManager
from profiling_bean.db_dto.ge_task_dto import GeTaskDto


class ParseAiCoreOpSummary:
    """
    handler ai core op data and get a summary data sheet
    """
    TASK_TIME_COL_NUM = 8
    TRAIN_TASK_TIME_COL_NUM = 7
    TABLE_PATH = ConfigManager.TABLES
    TABLES_PATH = ConfigManager.TABLES

    def __init__(self: any, sample_config: dict) -> None:
        self.sample_config = sample_config
        self.project_path = self.sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self.iter_range = self.sample_config.get(StrConstant.PARAM_ITER_ID)
        self.conn = None
        self.curs = None

    def init_params(self: any) -> None:
        """
        initial and check params
        :return: None
        """
        if self.project_path is None or not os.path.exists(self.project_path):
            logging.error("Failed to get project path.")
            raise ProfException(ProfException.PROF_SYSTEM_EXIT)
        sql_dir = PathManager.get_sql_dir(self.project_path)
        if not os.path.exists(sql_dir):
            logging.error("Failed to get sqlite path.")
            raise ProfException(ProfException.PROF_SYSTEM_EXIT)
        if os.path.exists(PathManager.get_db_path(self.project_path, DBNameConstant.DB_AICORE_OP_SUMMARY)):
            os.remove(PathManager.get_db_path(self.project_path, DBNameConstant.DB_AICORE_OP_SUMMARY))

    def create_summary_table(self: any) -> None:
        """
        store ge graph and task data in ge_info.db
        :return: None
        """
        if not DBManager.check_tables_in_db(self.get_db_path(DBNameConstant.DB_GE_INFO),
                                            DBNameConstant.TABLE_GE_TASK):
            logging.warning("maybe the data of framework is not collected."
                            "try to export data with no framework data.")
            if not DBManager.check_tables_in_db(self.get_db_path(DBNameConstant.DB_RUNTIME),
                                                DBNameConstant.TABLE_METRICS_SUMMARY):
                logging.warning("No need to create db for op summary, "
                                "maybe the data of aicore is not collected.")
                return
        self.create_conn()
        if not (self.conn and self.curs):
            return
        self.create_ge_summary_table()
        self.create_ge_tensor_table()
        self.create_ai_core_metrics_table()
        self.create_task_time_table()
        DBManager.destroy_db_connect(self.conn, self.curs)

    def create_conn(self: any) -> None:
        """
        create connection
        :return: connect and cursor
        """
        conn_path = self.get_db_path(DBNameConstant.DB_AICORE_OP_SUMMARY)
        self.conn, self.curs = DBManager.create_connect_db(conn_path)
        os.chmod(conn_path, NumberConstant.FILE_AUTHORITY)

    def create_ge_summary_table(self: any) -> None:
        """
        create ge summary table
        :return: None
        """
        if not DBManager.check_tables_in_db(self.get_db_path(DBNameConstant.DB_GE_INFO), DBNameConstant.TABLE_GE_TASK):
            logging.warning("unable to create ge summary table, because table %s is not found.",
                            DBNameConstant.TABLE_GE_TASK)
            return
        if not DBManager.attach_to_db(self.conn, self.project_path, DBNameConstant.DB_GE_INFO, "ge_info"):
            logging.warning("unable to create ge summary table, because attach db of ge failed.")
            return
        ge_create_sql = DBManager.sql_create_general_table("GeSummaryMap",
                                                           DBNameConstant.TABLE_SUMMARY_GE, self.TABLES_PATH)
        DBManager.execute_sql(self.conn, ge_create_sql)
        ge_data = self._get_ge_data()
        DBManager.insert_data_into_table(self.conn, DBNameConstant.TABLE_SUMMARY_GE, ge_data)

    def create_ge_tensor_table(self: any) -> None:
        """
        create ge tensor table
        """
        if not DBManager.check_tables_in_db(self.get_db_path(DBNameConstant.DB_GE_INFO),
                                            DBNameConstant.TABLE_GE_TENSOR):
            logging.warning("unable to create ge tensor table, because table %s is not found.",
                            DBNameConstant.TABLE_GE_TENSOR)
            return
        ge_tensor_create_sql = DBManager.sql_create_general_table("GeTensorMap",
                                                                  DBNameConstant.TABLE_SUMMARY_TENSOR, self.TABLES_PATH)
        DBManager.execute_sql(self.conn, ge_tensor_create_sql)
        ge_data = []
        iter_list = MsprofIteration(self.project_path).get_index_id_list_with_index_and_model(self.iter_range)
        ge_tensor_sql = f"select * from {DBNameConstant.TABLE_GE_TENSOR} where index_id=? and model_id=?"
        for index_and_model in iter_list:
            ge_data.extend(DBManager.fetch_all_data(self.curs, ge_tensor_sql, index_and_model))
        DBManager.insert_data_into_table(self.conn, DBNameConstant.TABLE_SUMMARY_TENSOR, ge_data)

    def create_ai_core_metrics_table(self: any) -> None:
        """
        create ai core metrics table
        :return: None
        """
        db_name = os.path.splitext(DBNameConstant.DB_RUNTIME)[0]
        if not DBManager.attach_to_db(self.conn, self.project_path, DBNameConstant.DB_RUNTIME, db_name):
            logging.warning("unable to create ai core metrics table, because attach db of runtime failed.")
            return
        if DBManager.check_tables_in_db(self.get_db_path(DBNameConstant.DB_RUNTIME),
                                        CommonConstant.METRICS_SUMMARY_TABLE):
            sql = "create table if not exists ai_core_metrics " \
                  "as select * from {0}.{1}".format(db_name,
                                                    CommonConstant.METRICS_SUMMARY_TABLE)
            if ChipManager().is_chip_v4() and not ProfilingScene().is_operator():
                iter_time = MsprofIteration(self.project_path).get_iter_interval(self.iter_range)
                sql = "create table if not exists ai_core_metrics " \
                      "as select * from {0}.{1} where start_time>{2} and end_time<{3}" \
                    .format(db_name, CommonConstant.METRICS_SUMMARY_TABLE, iter_time[0], iter_time[1])
        elif DBManager.check_tables_in_db(self.get_db_path(DBNameConstant.DB_RUNTIME),
                                          CommonConstant.AIV_METRICS_SUMMARY_TABLE):
            sql = "create table if not exists ai_core_metrics " \
                  "as select * from {0}.{1}".format(db_name,
                                                    CommonConstant.AIV_METRICS_SUMMARY_TABLE)
        else:
            logging.warning("unable to create ai core metrics table, because table is not found.")
            return
        DBManager.execute_sql(self.conn, sql)

    def create_task_time_table(self: any) -> None:
        """
        create task time table
        :return: true or false
        """
        create_table_sql = DBManager.sql_create_general_table("ModifiedTaskTimeMap", "task_time",
                                                              self.TABLE_PATH)
        if not create_table_sql:
            logging.error("unable to create task time table, generate sql statement failed!")
            return
        DBManager.execute_sql(self.conn, create_table_sql)
        data = self.get_task_time_data()
        if not data:
            logging.warning("unable to create task time table, because no task data found.")
            return
        insert_sql = 'insert or ignore into {0} ' \
                     'values ({value})'.format(DBNameConstant.TABLE_SUMMARY_TASK_TIME,
                                               value="?," * (len(data[0]) - 1) + "?")
        DBManager.executemany_sql(self.conn, insert_sql, data)

    def get_task_time_data(self: any) -> list:
        """
        get task time data
        :return: task data list
        """
        ge_data = self._get_ge_data_from_summary()
        project_path = self.sample_config.get("result_dir")
        if AiStackDataCheckManager.contain_task_time_data(project_path):
            fetch_data = GetOpTableTsTime(self.sample_config).get_task_time_data(ge_data)
            task_data = OpCommonFunc.calculate_task_time(fetch_data)
            return task_data
        return []

    def get_db_path(self: any, db_name: str) -> str:
        """
        get database path
        :param db_name: db name
        :return: path of db
        """
        return PathManager.get_db_path(self.project_path, db_name)

    def run(self: any) -> None:
        """
        entry for analysis op summary data
        :return: None
        """
        try:
            self.init_params()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError, ProfException) as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
            return
        try:
            self.create_summary_table()
        except sqlite3.Error as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)

    def _get_ge_data_from_summary(self: any) -> list:
        if not DBManager.judge_table_exist(self.curs, DBNameConstant.TABLE_SUMMARY_GE):
            return []
        ge_sql = "SELECT task_type, stream_id, task_id, batch_id, context_id from {0}".format(
            DBNameConstant.TABLE_SUMMARY_GE)
        return DBManager.fetch_all_data(self.curs, ge_sql, dto_class=GeTaskDto)

    def _get_ge_data(self: any) -> list:
        ge_data = []
        iter_list = MsprofIteration(self.project_path).get_index_id_list_with_index_and_model(self.iter_range)
        ge_sql = f"SELECT model_id, batch_id, task_id, stream_id, " \
                 f"op_name, op_type, block_dim, mix_block_dim, task_type, timestamp, index_id, context_id " \
                 f"from {DBNameConstant.TABLE_GE_TASK} where index_id=? and model_id=?"
        for index_and_model in iter_list:
            ge_data.extend(DBManager.fetch_all_data(self.curs, ge_sql, index_and_model))
        return ge_data

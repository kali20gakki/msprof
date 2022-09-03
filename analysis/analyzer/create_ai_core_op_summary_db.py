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
from common_func.msprof_exception import ProfException
from common_func.msprof_iteration import MsprofIteration
from common_func.msvp_constant import MsvpConstant
from common_func.path_manager import PathManager


class ParseAiCoreOpSummary:
    """
    handler ai core op data and get a summary data sheet
    """
    TASK_TIME_COL_NUM = 8
    TRAIN_TASK_TIME_COL_NUM = 7
    TABLE_PATH = os.path.join(MsvpConstant.CONFIG_PATH, 'Tables.ini')
    TABLES_PATH = os.path.join(MsvpConstant.CONFIG_PATH, 'Tables.ini')

    def __init__(self: any, sample_config: dict) -> None:
        self.sample_config = sample_config
        self.project_path = self.sample_config.get("result_dir")
        self.iter_id = self.sample_config.get("iter_id")
        self.model_id = self.sample_config.get("model_id")

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
        conn, curs = self.create_conn()
        if not (conn and curs):
            return
        self.create_ge_summary_table(conn)
        self.create_ge_tensor_table(conn)
        self.create_ai_core_metrics_table(conn)
        self.create_task_time_table(conn)
        DBManager.destroy_db_connect(conn, curs)

    def create_conn(self: any) -> tuple:
        """
        create connection
        :return: connect and cursor
        """
        conn_path = self.get_db_path(DBNameConstant.DB_AICORE_OP_SUMMARY)
        conn = sqlite3.connect(conn_path)
        curs = conn.cursor()
        os.chmod(conn_path, NumberConstant.FILE_AUTHORITY)
        return conn, curs

    def create_ge_summary_table(self: any, conn: any) -> None:
        """
        create ge summary table
        :param conn: sqlite curs
        :return: None
        """
        if not DBManager.check_tables_in_db(self.get_db_path(DBNameConstant.DB_GE_INFO), DBNameConstant.TABLE_GE_TASK):
            logging.warning("unable to create ge summary table, because table %s is not found.",
                            DBNameConstant.TABLE_GE_TASK)
            return
        if not DBManager.attach_to_db(conn, self.project_path, DBNameConstant.DB_GE_INFO, "ge_info"):
            logging.warning("unable to create ge summary table, because attach db of ge failed.")
            return
        ge_create_sql = DBManager.sql_create_general_table("GeSummaryMap",
                                                           DBNameConstant.TABLE_SUMMARY_GE, self.TABLES_PATH)
        DBManager.execute_sql(conn, ge_create_sql)
        ge_data = self._get_ge_data(conn)
        DBManager.insert_data_into_table(conn, DBNameConstant.TABLE_SUMMARY_GE, ge_data)

    def create_ge_tensor_table(self: any, conn: any) -> None:
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
        DBManager.execute_sql(conn, ge_tensor_create_sql)
        ge_data = []
        iter_dict = MsprofIteration(self.project_path).get_iter_dict_with_index_and_model(self.iter_id, self.model_id)
        ge_tensor_sql = "select * from {0} where " \
                        "(index_id=? or index_id=0) and model_id=?" \
            .format(DBNameConstant.TABLE_GE_TENSOR)
        for index_and_model in iter_dict.values():
            ge_data.extend(DBManager.fetch_all_data(conn.cursor(), ge_tensor_sql, index_and_model))

        DBManager.insert_data_into_table(conn, DBNameConstant.TABLE_SUMMARY_TENSOR, ge_data)

    def create_ai_core_metrics_table(self: any, conn: any) -> None:
        """
        create ai core metrics table
        :param conn: sqlite curs
        :return: None
        """
        db_name = os.path.splitext(DBNameConstant.DB_RUNTIME)[0]
        if not DBManager.attach_to_db(conn, self.project_path, DBNameConstant.DB_RUNTIME, db_name):
            logging.warning("unable to create ai core metrics table, because attach db of runtime failed.")
            return
        if DBManager.check_tables_in_db(self.get_db_path(DBNameConstant.DB_RUNTIME),
                                        CommonConstant.METRICS_SUMMARY_TABLE):
            sql = "create table if not exists ai_core_metrics " \
                  "as select * from {0}.{1}".format(db_name,
                                                    CommonConstant.METRICS_SUMMARY_TABLE)
        elif DBManager.check_tables_in_db(self.get_db_path(DBNameConstant.DB_RUNTIME),
                                          CommonConstant.AIV_METRICS_SUMMARY_TABLE):
            sql = "create table if not exists ai_core_metrics " \
                  "as select * from {0}.{1}".format(db_name,
                                                    CommonConstant.AIV_METRICS_SUMMARY_TABLE)
        else:
            logging.warning("unable to create ai core metrics table, because table is not found.")
            return
        DBManager.execute_sql(conn, sql)

    def create_task_time_table(self: any, conn: any) -> None:
        """
        create task time table
        :param conn: sqlite conn
        :return: true or false
        """
        create_table_sql = DBManager.sql_create_general_table("ModifiedTaskTimeMap", "task_time",
                                                              self.TABLE_PATH)
        if not create_table_sql:
            logging.error("unable to create task time table, generate sql statement failed!")
            return
        DBManager.execute_sql(conn, create_table_sql)
        data = self.get_task_time_data()
        if not data:
            logging.warning("unable to create task time table, because no task data found.")
            return
        insert_sql = 'insert or ignore into {0} ' \
                     'values ({value})'.format(DBNameConstant.TABLE_SUMMARY_TASK_TIME,
                                               value="?," * (len(data[0]) - 1) + "?")
        DBManager.executemany_sql(conn, insert_sql, data)

    def get_task_time_data(self: any) -> list:
        """
        get task time data
        :return: task data list
        """
        project_path = self.sample_config.get("result_dir")
        if AiStackDataCheckManager.contain_task_time_data(project_path):
            fetch_data = GetOpTableTsTime(self.sample_config).get_task_time_data()
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

    def _get_ge_data(self: any, conn: any) -> list:
        ge_data = []
        iter_dict = MsprofIteration(self.project_path).get_iter_dict_with_index_and_model(self.iter_id, self.model_id)
        ge_sql = "SELECT model_id, batch_id, task_id, stream_id, " \
                 "op_name, op_type, block_dim, task_type, timestamp, index_id from {0} where " \
                 "(index_id=? or index_id=0) and model_id=?" \
            .format(DBNameConstant.TABLE_GE_TASK)
        for index_and_model in iter_dict.values():
            ge_data.extend(DBManager.fetch_all_data(conn.cursor(), ge_sql, index_and_model))
        return ge_data

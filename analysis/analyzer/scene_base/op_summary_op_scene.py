"""
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""

import logging
import os

from analyzer.get_op_table_task_time import GetOpTableTsTime
from analyzer.op_common_function import OpCommonFunc
from common_func.ai_stack_data_check_manager import AiStackDataCheckManager
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.msvp_constant import MsvpConstant
from common_func.path_manager import PathManager


class OpSummaryOpScene:
    """
    op summary for single op
    """
    OP_TABLE_PATH = os.path.join(MsvpConstant.CONFIG_PATH, 'tables_operator.ini')
    TASK_TIME_COL_NUM = 7

    def __init__(self: any, sample_config: dict) -> None:
        self.sample_config = sample_config
        self.project_path = sample_config.get("result_dir")

    @staticmethod
    def _get_ge_sql() -> str:
        ge_sql = "SELECT model_id, task_id, stream_id, " \
                 "op_name, op_type, block_dim, task_type, timestamp, batch_id from {0} " \
            .format(DBNameConstant.TABLE_GE_TASK)
        return ge_sql

    @staticmethod
    def _create_core_table(curs: any, table_name: str) -> str:
        cols_infos = []
        cols_with_type = DBManager.get_table_info(curs, table_name)
        for col, col_type in cols_with_type.items():
            cols_infos.append("[{}] {}".format(col, col_type))
        return ",".join(cols_infos)

    @staticmethod
    def _get_tensor_data(conn: any) -> list:
        ge_tensor_sql = "select * from {}".format(DBNameConstant.TABLE_GE_TENSOR)
        tensor_data = DBManager.fetch_all_data(conn.cursor(), ge_tensor_sql)
        return tensor_data

    def create_ge_summary_table(self: any, conn: any) -> bool:
        """
        create ge summary table
        """
        ge_db_path = PathManager.get_db_path(self.project_path, DBNameConstant.DB_GE_INFO)
        if not DBManager.check_tables_in_db(ge_db_path, DBNameConstant.TABLE_GE_TASK):
            return False

        ge_merge_data = self._get_ge_merge_data()
        if not ge_merge_data:
            return False

        create_ge_summary_sql = DBManager.sql_create_general_table("SummaryGeMap", DBNameConstant.TABLE_SUMMARY_GE,
                                                                   self.OP_TABLE_PATH)
        DBManager.execute_sql(conn, create_ge_summary_sql)

        insert_sql = "insert into {0} " \
                     "values({value})".format(DBNameConstant.TABLE_SUMMARY_GE,
                                              value="?," * (len(ge_merge_data[0]) - 1) + "?")
        DBManager.executemany_sql(conn, insert_sql, ge_merge_data)
        return True

    def create_ge_tensor_table(self: any, conn: any) -> bool:
        """
        create ge tensor table
        """
        if not self._check_tensor_table(conn):
            return False
        tensor_data = self._get_tensor_data(conn)

        if not tensor_data:
            return False
        self._save_tensor_data(conn, tensor_data)
        return True

    def create_ai_core_metrics_table(self: any, conn: any) -> bool:
        """
        create ai core metrics table
        """
        db_path = PathManager.get_db_path(self.project_path, DBNameConstant.DB_RUNTIME)
        if DBManager.check_tables_in_db(db_path, DBNameConstant.TABLE_AI_CORE_METRIC_SUMMARY):
            core_merge_data = self._get_ai_core_metric(conn, DBNameConstant.TABLE_AI_CORE_METRIC_SUMMARY)
        elif DBManager.check_tables_in_db(db_path, DBNameConstant.TABLE_AIV_METRIC_SUMMARY):
            core_merge_data = self._get_ai_core_metric(conn, DBNameConstant.TABLE_AIV_METRIC_SUMMARY)
        else:
            return False
        if core_merge_data:
            insert_sql = "insert into {0} " \
                         "values({value})".format(DBNameConstant.TABLE_SUMMARY_METRICS,
                                                  value="?," * (len(core_merge_data[0]) - 1) + "?")
            DBManager.executemany_sql(conn, insert_sql, core_merge_data)
            return True
        return False

    def get_task_time_data(self: any) -> list:
        """
        get task time data
        """
        project_path = self.sample_config.get("result_dir")
        if AiStackDataCheckManager.contain_task_time_data(project_path):
            fetch_data = GetOpTableTsTime(self.sample_config).get_task_time_data()
            task_data = OpCommonFunc.calculate_task_time(fetch_data)
            return task_data
        return []

    def create_task_time_table(self: any, conn: any) -> bool:
        """
        create task time table
        """
        create_table_sql = DBManager.sql_create_general_table("ModifiedTaskTimeMap",
                                                              DBNameConstant.TABLE_SUMMARY_TASK_TIME,
                                                              self.OP_TABLE_PATH)
        DBManager.execute_sql(conn, create_table_sql)

        data = self.get_task_time_data()
        if not data:
            return False
        insert_sql = 'insert or ignore into {0} ' \
                     'values ({value})'.format(DBNameConstant.TABLE_SUMMARY_TASK_TIME,
                                               value="?," * (len(data[0]) - 1) + "?")
        DBManager.executemany_sql(conn, insert_sql, data)
        return True

    def create_summary_table(self: any) -> None:
        """
        store ge graph and task data in ge_info.db
        """
        ge_db_path = PathManager.get_db_path(self.project_path, DBNameConstant.DB_GE_INFO)
        if not DBManager.check_tables_in_db(ge_db_path,
                                            DBNameConstant.TABLE_GE_TASK):
            logging.warning("Try to export op summary without ge data, "
                            "maybe the data of framework is not collected.")
            if not DBManager.check_tables_in_db(
                    PathManager.get_db_path(self.project_path, DBNameConstant.DB_RUNTIME),
                    DBNameConstant.TABLE_METRICS_SUMMARY):
                logging.warning("No need to create db for op summary, "
                                "maybe the data of aicore is not collected.")
                return

        conn, curs = DBManager.create_connect_db(
            PathManager.get_db_path(self.project_path, DBNameConstant.DB_AICORE_OP_SUMMARY))
        if conn and curs:
            self._create_summary_table_helper(conn)
        DBManager.destroy_db_connect(conn, curs)

    def run(self: any) -> None:
        """
        run for op summary
        """
        if os.path.exists(PathManager.get_db_path(self.project_path, DBNameConstant.DB_AICORE_OP_SUMMARY)):
            return
        self.create_summary_table()

    def _get_ge_merge_data(self: any) -> list:
        ge_result = []
        ge_conn, ge_curs = DBManager.check_connect_db(self.project_path, DBNameConstant.DB_GE_INFO)
        if not (ge_conn and ge_curs):
            DBManager.destroy_db_connect(ge_conn, ge_curs)
            return ge_result
        ge_merge_sql = self._get_ge_sql()
        ge_result = DBManager.fetch_all_data(ge_curs, ge_merge_sql)
        DBManager.destroy_db_connect(ge_conn, ge_curs)
        return ge_result

    def _get_ai_core_metric(self: any, conn: any, table_name: str) -> list:
        core_data = []
        core_conn, core_curs = DBManager.check_connect_db(self.project_path, DBNameConstant.DB_RUNTIME)
        if not (core_conn and core_curs):
            DBManager.destroy_db_connect(core_conn, core_curs)
            return core_data
        sql = "create table if not exists {0} (".format(DBNameConstant.TABLE_SUMMARY_METRICS) \
              + self._create_core_table(core_curs, table_name) + ")"
        DBManager.execute_sql(conn, sql)

        sql = "select * from {0}".format(table_name)
        core_data = DBManager.fetch_all_data(core_curs, sql)
        DBManager.destroy_db_connect(core_conn, core_curs)
        return core_data

    def _check_tensor_table(self: any, conn: any) -> bool:
        if not DBManager.check_tables_in_db(PathManager.get_db_path(self.project_path, DBNameConstant.DB_GE_INFO),
                                            DBNameConstant.TABLE_GE_TENSOR):
            return False
        if not DBManager.attach_to_db(conn, self.project_path, DBNameConstant.DB_GE_INFO, "ge_info_attach"):
            logging.warning("unable to create ge tensor table, because attach db of ge failed.")
            return False
        return True

    def _save_tensor_data(self: any, conn: any, tensor_data: list) -> None:
        create_ge_tensor_sql = DBManager.sql_create_general_table("TensorGeMap",
                                                                  DBNameConstant.TABLE_SUMMARY_TENSOR,
                                                                  self.OP_TABLE_PATH)
        DBManager.execute_sql(conn, create_ge_tensor_sql)

        insert_sql = "insert into {0} " \
                     "values({value})".format(DBNameConstant.TABLE_SUMMARY_TENSOR,
                                              value="?," * (len(tensor_data[0]) - 1) + "?")
        DBManager.executemany_sql(conn, insert_sql, tensor_data)

    def _create_summary_table_helper(self: any, conn: any) -> None:
        if not self.create_ge_summary_table(conn):
            logging.warning("unable to create ge summary table")
        if not self.create_ge_tensor_table(conn):
            logging.warning("unable to create ge tensor table")
        if not self.create_ai_core_metrics_table(conn):
            logging.warning("unable to create ai core metrics table")
        if not self.create_task_time_table(conn):
            logging.warning("unable to create task time table")

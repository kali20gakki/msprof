"""
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""

import logging
import os
import sqlite3

from analyzer.get_op_table_task_time import GetOpTableTsTime
from common_func.common import CommonConstant
from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msvp_constant import MsvpConstant
from common_func.path_manager import PathManager
from common_func.platform.chip_manager import ChipManager


class OpCounterOpScene:
    """
    op counter for single op
    """
    OP_TABLE_PATH = os.path.join(MsvpConstant.CONFIG_PATH, 'tables_operator.ini')

    def __init__(self: any, sample_config: dict) -> None:
        self.sample_config = sample_config
        self.project_path = sample_config.get("result_dir")
        self.iter_id = sample_config.get("iter_id")

    @staticmethod
    def _get_ge_sql() -> str:
        ge_sql = 'select model_id, op_name, op_type, task_type, ' \
                 'task_id, stream_id, batch_id from {0}'.format(DBNameConstant.TABLE_GE_TASK)
        return ge_sql

    @staticmethod
    def _get_op_report_sql() -> str:
        sql = "select op_type, {0}.task_type, count(op_type), sum(duration) as total_time, " \
              "min(duration) as min, sum(duration)/count(op_type) as avg, max(duration) as max" \
              " from {0}, {1} where {0}.task_id={1}.task_id and {0}.stream_id={1}.stream_id " \
              "and {0}.batch_id={1}.batch_id and {0}.task_type={1}.task_type " \
              "group by op_type,{0}.task_type" \
            .format(CommonConstant.GE_TASK_MEGED_TABLE, DBNameConstant.TABLE_OP_COUNTER_RTS_TASK)
        return sql

    @staticmethod
    def _cal_total(task_data: list) -> int:
        """
        calculate total time for each device
        :param task_data: {op_type, task_type, count of op_type, total_time of duration,
        min of duration, avg of duration, max of duration}
        :return: total time
        """
        total_time = 0
        for _data in task_data:
            total_time += _data[3]  # the index of total time
        return total_time

    def run(self: any) -> None:
        """
        run for op statics
        :return: None
        """
        if os.path.exists(PathManager.get_db_path(self.project_path, DBNameConstant.DB_OP_COUNTER)):
            return
        if not self._is_db_need_to_create():
            logging.warning("No need to create db for op counter for operator scene, "
                            "maybe the data of framework or task is not collected.")
            return
        db_path = PathManager.get_db_path(self.project_path, DBNameConstant.DB_OP_COUNTER)
        merge_conn, merge_curs = DBManager.create_connect_db(db_path)
        if not merge_conn or not merge_curs:
            logging.warning("unable to create op counter db connection")
            return
        self._create_db(merge_conn)
        self.create_ge_merge(merge_conn)
        self.create_task(merge_conn)
        self.create_report(merge_conn, merge_curs)
        DBManager.destroy_db_connect(merge_conn, merge_curs)

    def create_ge_merge(self: any, merge_conn: any) -> None:
        """
        merge GE ge_task_data ge_graph_data into merged db
        :param merge_conn:
        :return: None
        """
        db_path = PathManager.get_db_path(self.project_path, DBNameConstant.DB_GE_INFO)
        ge_conn, ge_curs = DBManager.check_connect_db_path(db_path)
        if ge_conn and ge_curs and DBManager.judge_table_exist(ge_curs, DBNameConstant.TABLE_GE_TASK):
            ge_data = DBManager.fetch_all_data(ge_curs, self._get_ge_sql())
            DBManager.insert_data_into_table(merge_conn, CommonConstant.GE_TASK_MEGED_TABLE, ge_data)
        DBManager.destroy_db_connect(ge_conn, ge_curs)

    def create_task(self: any, merge_conn: any) -> None:
        """
        insert data to task time table
        :param merge_conn:
        :return:
        """
        rts_data = GetOpTableTsTime(self.sample_config).get_task_time_data()
        try:
            DBManager.insert_data_into_table(merge_conn, CommonConstant.RTS_TASK_TABLE, rts_data)
        except sqlite3.Error as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)

    def create_report(self: any, merge_conn: any, merge_curs: any) -> None:
        """
        create report table
        :param merge_conn:
        :param merge_curs:
        :return: None
        """
        sql = self._get_op_report_sql()
        task_data = DBManager.fetch_all_data(merge_curs, sql)
        if not task_data:
            return
        total_time = self._cal_total(task_data)
        total_data = []
        for _data in task_data:
            ratio = 0
            if total_time > 0:
                ratio = round(float(_data[3] / total_time * CommonConstant.PERCENT), NumberConstant.DECIMAL_ACCURACY)
            total_data.append(
                (_data[0], _data[1], _data[2],
                 round(float(_data[3]), NumberConstant.DECIMAL_ACCURACY),
                 round(float(_data[4]), NumberConstant.DECIMAL_ACCURACY),
                 round(float(_data[5]), NumberConstant.DECIMAL_ACCURACY),
                 round(float(_data[6]), NumberConstant.DECIMAL_ACCURACY),
                 ratio))
        if total_data:
            sorted_total_data = sorted(total_data, key=lambda x: x[7], reverse=True)
            DBManager.insert_data_into_table(merge_conn, DBNameConstant.TABLE_OP_COUNTER_OP_REPORT, sorted_total_data)

    def _is_db_need_to_create(self: any) -> bool:
        ge_db_path = PathManager.get_db_path(self.project_path, DBNameConstant.DB_GE_INFO)
        if DBManager.check_tables_in_db(ge_db_path, DBNameConstant.TABLE_GE_TASK):
            return True
        if ChipManager().is_chip_v1():
            return DBManager.check_tables_in_db(PathManager.get_db_path(self.project_path, DBNameConstant.DB_RUNTIME),
                                                DBNameConstant.TABLE_RUNTIME_TASK_TIME)
        return DBManager.check_tables_in_db(PathManager.get_db_path(self.project_path, DBNameConstant.DB_HWTS),
                                            DBNameConstant.TABLE_HWTS_TASK_TIME)

    def _create_db(self: any, merge_conn: any) -> None:

        ge_create_sql = DBManager.sql_create_general_table("GeMergeMap", DBNameConstant.TABLE_OP_COUNTER_GE_MERGE,
                                                           self.OP_TABLE_PATH)
        DBManager.execute_sql(merge_conn, ge_create_sql)

        rts_task_create_sql = DBManager.sql_create_general_table("RtsTaskMap",
                                                                 DBNameConstant.TABLE_OP_COUNTER_RTS_TASK,
                                                                 self.OP_TABLE_PATH)
        DBManager.execute_sql(merge_conn, rts_task_create_sql)

        op_report_create_sql = DBManager.sql_create_general_table("OpReportMap",
                                                                  DBNameConstant.TABLE_OP_COUNTER_OP_REPORT,
                                                                  self.OP_TABLE_PATH)
        DBManager.execute_sql(merge_conn, op_report_create_sql)

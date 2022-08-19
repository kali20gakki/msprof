#!/usr/bin/python3
# coding=utf-8
"""
function: this script used to operate ts track
Copyright Huawei Technologies Co., Ltd. 2021. All rights reserved.
"""
from abc import ABC

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from msmodel.interface.base_model import BaseModel


class TsTrackModel(BaseModel, ABC):
    """
    acsq task model class
    """
    TS_AI_CPU_TYPE = 1

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

    def get_ai_cpu_data(self: any) -> list:
        """
        get ai cpu data
        """
        sql = "select stream_id, task_id, timestamp, task_state from {0} where task_type={1} " \
              "order by timestamp".format(DBNameConstant.TABLE_TASK_TYPE, self.TS_AI_CPU_TYPE)

        ai_cpu_with_state = DBManager.fetch_all_data(self.cur, sql)
        return ai_cpu_with_state

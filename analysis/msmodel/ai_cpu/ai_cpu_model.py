#!/usr/bin/python3
# coding=utf-8
"""
function: this script used to operate AI_CPU
Copyright Huawei Technologies Co., Ltd. 2021. All rights reserved.
"""

from common_func.db_name_constant import DBNameConstant
from common_func.db_manager import DBManager
from msmodel.interface.parser_model import ParserModel


class AiCpuModel(ParserModel):
    """
    ai_cpu model class
    """

    def __init__(self: any, result_dir: str, table_list: list) -> None:
        super().__init__(result_dir, DBNameConstant.DB_AI_CPU, table_list)

    def flush(self: any, data_list: list, ai_cpu_table_name: str = DBNameConstant.TABLE_AI_CPU) -> None:
        """
        insert data to table
        :param data_list: ai_cpu data
        :param ai_cpu_table_name: ai cpu table name
        :return:
        """
        self.insert_data_to_db(ai_cpu_table_name, data_list)

    def create_table(self: any) -> None:
        """
        create table
        """
        for table_name in self.table_list:
            if DBManager.judge_table_exist(self.cur, table_name):
                self.drop_table(table_name)
            table_map = "{0}Map".format(table_name)
            sql = DBManager.sql_create_general_table(table_map, table_name, self.TABLES_PATH)
            DBManager.execute_sql(self.conn, sql)

    def get_ai_cpu_data(self: any, table_name: str) -> list:
        """
        get all data from db
        :param table_name:
        :return:
        """
        if not DBManager.judge_table_exist(self.cur, table_name):
            return []
        all_data_sql = "select stream_id, task_id, sys_start, sys_end, batch_id from {}".format(table_name)
        return DBManager.fetch_all_data(self.cur, all_data_sql)

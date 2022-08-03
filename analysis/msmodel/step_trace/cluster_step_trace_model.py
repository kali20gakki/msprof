# !/usr/bin/python
# coding=utf-8
"""
Copyright Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
"""

from common_func.db_manager import DBManager
from msmodel.interface.base_model import BaseModel


class ClusterStepTraceModel(BaseModel):
    FORMAT_TABLE_NAME = "ClusterStepTrace"

    def __init__(self: any, result_dir: str, db_name: str, table_list: list):
        super().__init__(result_dir, db_name, table_list)

    def create_table(self: any) -> None:
        """
        create table
        """
        table_map = "{0}Map".format(ClusterStepTraceModel.FORMAT_TABLE_NAME)
        for table_name in self.table_list:
            if DBManager.judge_table_exist(self.cur, table_name):
                DBManager.drop_table(self.conn, table_name)
            sql = DBManager.sql_create_general_table(table_map, table_name, self.TABLES_PATH)
            DBManager.execute_sql(self.conn, sql)

# !/usr/bin/python
# coding=utf-8
"""
This script is used to create step trace database model for cluster.
Copyright Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
"""

import os

from common_func.db_manager import DBManager
from common_func.path_manager import PathManager
from msmodel.interface.base_model import BaseModel


class ClusterStepTraceModel(BaseModel):
    """
    Step trace model for cluster scene.
    """
    FORMAT_TABLE_NAME = "ClusterStepTrace"

    def __init__(self: any, result_dir: str, db_name: str, table_list: list) -> None:
        super().__init__(result_dir, db_name, table_list)

    def create_table(self: any) -> None:
        table_map = "{0}Map".format(ClusterStepTraceModel.FORMAT_TABLE_NAME)
        for table_name in self.table_list:
            if DBManager.judge_table_exist(self.cur, table_name):
                DBManager.drop_table(self.conn, table_name)
            sql = DBManager.sql_create_general_table(table_map, table_name, self.TABLES_PATH)
            DBManager.execute_sql(self.conn, sql)

    def init(self: any) -> bool:
        db_path = PathManager.get_db_path(self.result_dir, self.db_name)
        if os.path.exists(db_path):
            os.remove(db_path)
        return super().init()

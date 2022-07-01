#!/usr/bin/python3
# coding=utf-8
"""
function: this script used to operate ts track
Copyright Huawei Technologies Co., Ltd. 2021. All rights reserved.
"""
from abc import ABC

from common_func.db_manager import DBManager
from msmodel.interface.base_model import BaseModel


class TsTrackModel(BaseModel, ABC):
    """
    acsq task model class
    """

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

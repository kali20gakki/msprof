#!/usr/bin/python3
# coding=utf-8
"""
function: this script used to operate data queue
Copyright Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
"""

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from msmodel.interface.parser_model import ParserModel


class DataQueueModel(ParserModel):
    """
    data queue model class
    """

    def __init__(self: any, result_dir: str, table_list: list) -> None:
        super().__init__(result_dir, DBNameConstant.DB_CLUSTER_DATA_PREPROCESS, table_list)

    def create_table(self: any) -> None:
        """
        create table
        """
        for table_name in self.table_list:
            if DBManager.judge_table_exist(self.cur, table_name):
                continue
            sql = DBManager.sql_create_general_table('{}Map'.format(table_name), table_name, self.TABLES_PATH)
            DBManager.execute_sql(self.conn, sql)

    def flush(self: any, data_list: list) -> None:
        """
        insert data to table
        :param data_list: data queue
        :return:
        """
        self.insert_data_to_db(DBNameConstant.TABLE_DATA_QUEUE, data_list)

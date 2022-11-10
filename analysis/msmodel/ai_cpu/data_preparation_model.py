#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from msmodel.interface.parser_model import ParserModel


class DataPreparationModel(ParserModel):
    """
    data preparation model class
    """

    def __init__(self: any, result_dir: str, table_list: list) -> None:
        super().__init__(result_dir, DBNameConstant.DB_CLUSTER_DATA_PREPROCESS, table_list)
        self._current_table_name = None

    def create_table(self: any) -> None:
        """
        create table
        """
        for table_name in self.table_list:
            if DBManager.judge_table_exist(self.cur, table_name):
                continue
            sql = DBManager.sql_create_general_table('{}Map'.format(table_name), table_name, self.TABLES_PATH)
            DBManager.execute_sql(self.conn, sql)

    def flush_all(self: any, data_dict: dict) -> None:
        """
        insert all data preparation data to table
        :param data_dict: collected data preparation data
        :return:
        """
        for table_name in data_dict.keys():
            self._current_table_name = table_name
            self.flush(data_dict.get(table_name, []))

    def flush(self: any, data_list: list) -> None:
        """
        insert data to table
        :param data_list: collected data preparation data
        :return:
        """
        if self._current_table_name:
            self.insert_data_to_db(self._current_table_name, data_list)

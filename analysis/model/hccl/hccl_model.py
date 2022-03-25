#!/usr/bin/python3
# coding=utf-8
"""
function: this script used to operate HCCL
Copyright Huawei Technologies Co., Ltd. 2021. All rights reserved.
"""
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from model.interface.parser_model import ParserModel


class HCCLModel(ParserModel):
    """
    acsq task model class
    """

    def __init__(self: any, result_dir: str, table_list: list) -> None:
        super().__init__(result_dir, DBNameConstant.DB_HCCL, table_list)

    def flush(self: any, data_list: list) -> None:
        """
        insert data to table
        :param data_list: hccl data
        :return:
        """
        self.insert_data_to_db(DBNameConstant.TABLE_HCCL_ALL_REDUCE, data_list)

    def get_hccl_timeline_data(self: any, sql: str) -> list:
        """
        get hccl data
        :param sql:
        :return:
        """
        return DBManager.fetch_all_data(self.cur, sql)

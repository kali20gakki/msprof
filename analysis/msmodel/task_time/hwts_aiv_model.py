#!/usr/bin/python3
# coding=utf-8
"""
function: this script used to record hwts log db
Copyright Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
"""
import os

from common_func.db_name_constant import DBNameConstant
from common_func.path_manager import PathManager
from msmodel.interface.parser_model import ParserModel


class HwtsAivModel(ParserModel):
    """
    class used to operate hwts log model
    """

    def __init__(self: any, result_dir: str, table_list: list) -> None:
        super(HwtsAivModel, self).__init__(result_dir, DBNameConstant.DB_HWTS_AIV,
                                           [DBNameConstant.TABLE_HWTS_TASK, DBNameConstant.TABLE_HWTS_TASK_TIME])
        self.table_list = table_list

    def flush(self: any, data_list: list) -> tuple:
        """
        flush to db
        :param data_list: data
        :return:
        """
        return data_list, self.table_list

    def flush_data(self: any, data_list: list, table_name: str) -> None:
        """
        flush to db
        :param data_list: data
        :param table_name: table name
        :return:
        """
        self.insert_data_to_db(table_name, data_list)

    def clear(self: any) -> None:
        """
        clear db file
        :return:
        """
        if os.path.exists(PathManager.get_db_path(self.result_dir, DBNameConstant.DB_HWTS_AIV)):
            os.remove(PathManager.get_db_path(self.result_dir, DBNameConstant.DB_HWTS_AIV))

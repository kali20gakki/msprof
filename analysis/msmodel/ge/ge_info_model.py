#!/usr/bin/python3
# coding=utf-8
"""
function: save ge info data to db
Copyright Huawei Technologies Co., Ltd. 2021. All rights reserved.
"""
from common_func.db_name_constant import DBNameConstant
from msmodel.interface.parser_model import ParserModel


class GeModel(ParserModel):
    """
    ge info model class
    """

    def __init__(self: any, result_dir: str, table_list: list) -> None:
        super().__init__(result_dir, DBNameConstant.DB_GE_INFO, table_list)
        self._current_table_name = None

    def flush_all(self: any, data_dict: dict) -> None:
        """
        insert all ge data to table
        :param data_dict: ge data
        :return:
        """
        for table_name in data_dict.keys():
            self._current_table_name = table_name
            self.flush(data_dict.get(table_name, []))

    def flush(self: any, data_list: list) -> None:
        """
        insert ge data into database
        """
        self.insert_data_to_db(self._current_table_name, data_list)

    def delete_table(self: any, table_name: str) -> None:
        """
        delete ge data
        """
        self.cur.execute('delete from {}'.format(table_name))

    def get_ge_model_name(self: any) -> any:
        """
        get ge model name
        """
        return self.__class__.__name__

#!/usr/bin/python3
# coding=utf-8
"""
function: runtime db operate for parsing
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""

from common_func.db_name_constant import DBNameConstant
from msmodel.interface.parser_model import ParserModel


class RuntimeApiModel(ParserModel):
    """
    db operator for runtime_api parser
    """

    @staticmethod
    def class_name() -> str:
        """
        class name
        """
        return 'RuntimeApiModel'

    def flush(self: any, data_list: list) -> None:
        """
        flush runtime_api data to db
        :param data_list:runtime_api data list
        :return: None
        """
        self.insert_data_to_db(DBNameConstant.TABLE_API_CALL, data_list)

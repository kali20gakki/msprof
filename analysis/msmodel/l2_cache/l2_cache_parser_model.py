#!/usr/bin/python3
# coding=utf-8
"""
function: l2 cache db operate for parsing
Copyright Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
"""

from common_func.db_name_constant import DBNameConstant
from msmodel.interface.parser_model import ParserModel


class L2CacheParserModel(ParserModel):
    """
    db operator for l2 cache parser
    """

    def __init__(self: any, result_dir: str) -> None:
        super(L2CacheParserModel, self).__init__(result_dir, DBNameConstant.DB_L2CACHE,
                                                 [DBNameConstant.TABLE_L2CACHE_PARSE])

    def flush(self: any, data_list: list) -> None:
        """
        insert data into database
        """
        self.insert_data_to_db(self.table_list[0], data_list)

    def get_class_name(self: any) -> str:
        """
        get class name
        """
        return self.__class__.__name__

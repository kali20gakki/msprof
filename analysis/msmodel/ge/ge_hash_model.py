#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

from common_func.db_name_constant import DBNameConstant
from msmodel.interface.parser_model import ParserModel


class GeHashModel(ParserModel):
    """
    ge hash model class
    """

    def __init__(self: any, result_dir: str, table_list: list) -> None:
        super().__init__(result_dir, DBNameConstant.DB_GE_HASH, table_list)
        self.table_list = table_list

    def flush(self: any, data_list: list) -> None:
        """
        insert data to table
        :param data_list: ge hash data
        :return:
        """
        self.insert_data_to_db(self.table_list[0], data_list)

    def get_ge_hash_model_name(self: any) -> any:
        """
        get ge hash model name
        """
        return self.__class__.__name__
